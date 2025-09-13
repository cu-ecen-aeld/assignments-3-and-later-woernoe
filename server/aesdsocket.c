/*
** aesdsocket.c -- a stream socket server 
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdarg.h>
#include <syslog.h>
#include <sys/ioctl.h>
#include <sys/stat.h>


#define PORT "9000"  // the port users will be connecting to

#define BACKLOG 10   // how many pending connections queue will hold

#define BUFFER_SIZE 1024  


#define false 0
#define true !false

const char* server_name = "aesdsocket";

const char* savefile_name = "/var/tmp/aesdsocketdata";

int server_fd;


// start with no daemon
int start_as_daemon = false;


// Running Flag
volatile sig_atomic_t running = 1;


// open children
pid_t chldren_pids[BACKLOG+1];


/*
** Write message to syslog
*/
void WriteLog(int priority, const char* format, ...)
{
    va_list args;
    va_start(args, format);

    // Syslog initialisieren with LOG_USER facility
    openlog("aesdsocket", LOG_PID | LOG_CONS, LOG_USER);  

    vsyslog(priority, format, args );

    closelog();
}

/*
** remember open child-pids
*/
int setpid(pid_t n_pid)
{
    
    for ( int i=0 ; i < BACKLOG; i++ ) {
        if (chldren_pids[i] == 0) {
            chldren_pids[i] = n_pid;
            return 0;
        }
    }
    
    return -1;
}

/*
** remove child-pid
*/
int clearpid(pid_t c_pid)
{
    for (int i=0 ; i < BACKLOG; i++ ) {
        if (chldren_pids[i] == c_pid) {
            chldren_pids[i] = 0;
            return 0;
        }
    }
 
    return -1;
}


/*
** Signalhandler for fork()
*/
void sigchld_handler(int s)
{
    (void)s; // quiet unused variable warning

    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    int status;
    pid_t pid;
    
       
    while((pid = waitpid(-1, &status, WNOHANG)) > 0) {
     
        clearpid(pid);   
        
        if( WIFEXITED(status)) {
            int code = WEXITSTATUS(status);

            if ( code == 255 ) {
 
	        //syslog(LOG_DEBUG, "sigchld_handler exit(-1) ");
                //closelog();
	        
                exit(-1);
            }
        }
    }

    errno = saved_errno;   
}


// Handler for SIGINT and SIGTERM
void handle_shutdown(int signo)
{

    (void)signo;

    // terminate main loop
    running = 0;
 
    shutdown(server_fd, SHUT_RDWR);
    
    // 2 sec delay
    //sleep(2);
    
    // look for open children
    for( int i=0; i< BACKLOG; i++ ) {
    
        if (chldren_pids[i] != 0) {
	   
           kill(chldren_pids[i], SIGTERM);
           
           chldren_pids[i] = 0;
        }
    
    }
}


/*
** Set Signal with sigaction
*/
int set_signal(int sig, void (*handler)(int), int flags) 
{

    struct sigaction sa;
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = flags;
    if (sigaction(sig, &sa, NULL) == -1 )
    {
        return( -1 );
    }
    
    return 0;
}




/*
** get sockaddr, IPv4 or IPv6: 
*/
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/*
** more bytes on input stream available
*/
int bytes_available(int fd)
{
     int bytes_available=0;

     // use ioctl to check for waiting characters
     ioctl(fd, FIONREAD, &bytes_available);

     return bytes_available;
}

/*
** Temporary for install daemon
*/
void handle_sigterm(int signo) 
{
 
    (void)signo;    
    
    exit(0);
}



/*
** start aestsocket as daemon
*/
int daemonize(int server_fd)
{
    // pid for forks
    pid_t pid;

    // 1st Fork
    pid = fork();
    if (pid < 0) {
        // Error on fork
        close( server_fd);
        exit(-1);
    }
    if (pid > 0) {
        // Parent
        close( server_fd);
        exit(0);
    }

    // child still alive
    
    // set sid
    if (setsid() < 0) {
        close( server_fd);
        exit(-1);
    }

    if ( (set_signal(SIGCHLD, SIG_IGN, 0) == -1) ||
         (set_signal(SIGHUP, SIG_IGN, 0) == -1) ||
         (set_signal(SIGTERM, handle_sigterm, 0) == -1) ) {
     
        close(server_fd);
        exit(-1);    
    }

    pid = fork();
    if (pid < 0 ) {
        close( server_fd);
        exit(-1);
    }
    if (pid > 0) {
        close( server_fd);
        exit(0);
     }

     // set up environment
     umask(0);
     chdir("/");

     // File descriptoren schliessen
     for (int fd = sysconf(_SC_OPEN_MAX); fd>=0; fd--) {
         if ( fd != server_fd ) {
             close(fd);
         }
     }

     return 0;
}



/* 
** main(): aesdscoket
*/
int main(int argc, char** argv)
{


    // server- / client- variables
    int client_fd;


    // addrinfos 
    struct addrinfo hints, *servinfo;
    struct sockaddr_storage client_addr;
    int yes = 1;
    socklen_t client_len;

    // has argument '-d' been added
    for (int i = 1; i < argc; i++) {
        if ( strcmp(argv[i], "-d" ) == 0 ) {
            
            // set variable for aesdsocket to be started as daemon
            start_as_daemon = true;
        }
    }

    // init/clear children pid list
    for (int i = 0; i < BACKLOG; i++ ) {
        chldren_pids[i] = 0;    // no valid pid
    }
    
   
    // init TCP Parameters: here only AF_INET been tested
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;        // alternatives AF_UNSPECIFIC or AF_INET6
    hints.ai_socktype = SOCK_STREAM;  // TCP
    hints.ai_flags = AI_PASSIVE;      // Server

    int status;
    if ((status = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0 ) {
        exit(-1);
    }

    // open (server-)socket
    server_fd = socket( servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if (server_fd == -1) {
       freeaddrinfo(servinfo);
       exit(-1);
    }

    // set socketopt
    if (setsockopt( server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1 ) {
       close(server_fd);
       freeaddrinfo(servinfo);
       exit(-1);
    }

    // bind to AF_INET
    if (bind( server_fd, servinfo->ai_addr, servinfo->ai_addrlen) == -1 ) {
       printf("Error: bind() - Port already assigned\n");
       close(server_fd);
       freeaddrinfo(servinfo);
       exit(-1);
    }

    // no longer in use
    freeaddrinfo(servinfo);


    // after bind: start daemon if requested
    if (start_as_daemon) {
        daemonize(server_fd);
    }
    
    
    // start listen
    if (listen(server_fd, BACKLOG) == -1) {
 
        close(server_fd);
        exit(-1);
    }

    // change signal handlers
    if ( (set_signal(SIGINT, handle_shutdown, 0) == -1) ||
         (set_signal(SIGTERM, handle_shutdown, 0) == -1) ||
         (set_signal(SIGCHLD, sigchld_handler, SA_RESTART) == -1) ) { 
         
         close(server_fd);
         exit(-1);
    }

    // loop for new connections    
    while( running ) {

        if(!start_as_daemon)
            printf("server: waiting for connections...\n");


        // accept connection
        client_len = sizeof client_addr;
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd == -1 ) {
            if (errno == EINTR) {    // not active with SA_RESTART
                continue;
            }
        
            close(server_fd);
            exit (-1);
        }

        // fork for client connection
        pid_t pid = fork();
        if (pid < 0) {

            close(client_fd);
            close(server_fd);
            exit(-1);
        }

        if (pid == 0) {  // child - part

            // server_fd no longer needed
            close(server_fd); 

            // Openlog again
            openlog("aesdsocket", LOG_PID | LOG_CONS, LOG_USER);  
            
	    // get IP address
            char ip_str[INET_ADDRSTRLEN];
            inet_ntop(client_addr.ss_family, get_in_addr((struct sockaddr *)&client_addr), ip_str, sizeof ip_str);

            // syslog report accepted connection
            syslog(LOG_DEBUG, "Accepted connection from %s", ip_str);
            
            if(!start_as_daemon)
                printf("Accepted connection from %s\n", ip_str);
            
            // receive buffer for malloc    
            char* recv_buffer = NULL;
            // temporary buffer for recv()
            char buffer[BUFFER_SIZE+1];
            // variables for handling recept stream
            ssize_t n;
            size_t total = 0;
            
            // as long as a connection is established
            while(true) {

                // collect received string
                do {
                    
                    // read received strem with max-size of BUFFER_SIZE 
                    if  ((n = recv(client_fd, buffer, BUFFER_SIZE, 0)) > 0 ) {
             
                        // terminate received string
                        buffer[n] = '\0';
                      
                        // assemble reception string
                        char* newBuffer = realloc(recv_buffer, total + n + 1);  // + 1: '\0'
                        
                        // Ignore if realloc()/malloc() fails
                        if (newBuffer) {
                            recv_buffer = newBuffer;
                            // append characters from last recv()
                            memcpy(recv_buffer + total, buffer, n);
                            total += n;
                            recv_buffer[total] = '\0';
                        }
                    }

                    // connection closed
                    if (n == 0 ) {     

                       syslog(LOG_DEBUG, "Closed connection from %s", ip_str);
                       if(!start_as_daemon)
                           printf("Close connection from %s\n", ip_str);
                       
                       // free received bytes
                       if (recv_buffer != NULL) {
                            free(recv_buffer);
                            recv_buffer = NULL;
                       }
 
                       close(client_fd);
                       closelog();
                       exit(0);
                    }
                    
                    // cancellation on recv() - e.g. SIGTERM
                    if (n == -1) {
                       // free received bytes
                       if (recv_buffer != NULL) {
                            free(recv_buffer);
                            recv_buffer = NULL;
                        }

                        close(client_fd);
                        closelog();
                        exit(-1);
                    }

                } while (bytes_available(client_fd) > 0);

            
                // recv_buffer should not be NULL if variable total is > 0
                if  (total > 0 && recv_buffer) {

                    // open save-file in append mode
                    FILE* wp = fopen(savefile_name, "a");
                    if (wp == NULL) {
                        free(recv_buffer);
                        close(client_fd);
                        closelog();
                       
                        exit(-1);
                    }

                    // write to save-file
                    if (fprintf(wp,"%s", recv_buffer) < 0) {
                        free(recv_buffer);
                        close(client_fd);
                        closelog();
                        exit(-1);
                    }

                    // close save-file
                    if (fclose(wp) == -1) {
                        free(recv_buffer);
                        close(client_fd);
                        closelog();
                        exit(-1);
                    }

                    // recv_buffer content no longer necessary
                    free(recv_buffer);
                    recv_buffer = NULL;
                    total = 0;

                    // Answer on received string
                    char readfile_buffer[BUFFER_SIZE+1]; // buffer could be reused

                    // open file
                    FILE* fp = fopen(savefile_name, "r");
                    if (fp == NULL) {
                        close(client_fd);
                        closelog();
                        exit(-1);
                    }

                    // read file line by line
                    while(fgets(readfile_buffer, BUFFER_SIZE, fp) != NULL) {

                        // send if length of line has characters
                        if (strlen(readfile_buffer) > 0 ) {
                            // send whole buffer
                            if (send(client_fd, readfile_buffer, strlen(readfile_buffer), 0) == -1) {

                                 close(client_fd);
                                 closelog();
                       
                                 exit(-1);
                            }
                        }
                    }

                    // close file
                    if (fclose(fp) == -1)
                    {
                         close(client_fd);
                         closelog();
                       
                         exit(-1);
                    }
                }
            }
 
            // out of reach
            closelog();
                       
            exit(0);
        }
        else {   // parent

            // parent has no control over client_fd
            close(client_fd);

            // remember child-pid
            setpid(pid);
        }

    }  // running

                
    // running == 0. closing server_fd
    close(server_fd);
    
    // remove savefile on close    
    if (unlink(savefile_name) != 0) {
        // notify error condition
        exit(-1);
    }

    //sleep(5);
    exit (0);

}



