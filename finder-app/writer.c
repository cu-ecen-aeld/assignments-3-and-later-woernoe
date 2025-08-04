#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>


//
// writer.c  
//
// call: writer <directory_file> <text>
//
// writes <text> to <directory_file>
//
int main(int argc, char*argv[]) {


    // Syslog initialisieren with LOG_USER facility
    openlog("writer", LOG_PID | LOG_CONS, LOG_USER);

    
// Check for calling arguments
    if (argc != 3) {
    
        // writing to syslog -> error not reported
        // printf("Error: %d arguments but call 'writer <filename> <text to write>'", argc - 1 );
        
        // report in syslog
        syslog(LOG_ERR, "Error: %d arguments but call 'writer <filename> <text to write>'", argc - 1 );
        closelog();

        return 1; 

    }

    // filename - first argument
    const char* filename = argv[1];
    // text2write - second argument
    const char* text2write = argv[2];

    // open file in (over-)write mode
    FILE* fp = fopen(filename, "w");
    
    if (fp == NULL) {     // Error on opening

        // writing to syslog -> error not reported
        // printf("Error on opening file %s", filename);
        
        // report to syslog
        syslog(LOG_ERR, "Error on opening file %s", filename);
        closelog();

        return 1;

    }

    // write to file
    if (fprintf( fp, "%s", text2write) < 0) {
        
        // <text> not written
        
        // writing to syslog -> error not reported
        // printf( "Error on writing to file '%s'", filename);
        
        syslog(LOG_ERR, "Error on writing to file '%s'", filename);
        closelog();
  
        return 1;
    }

    // close file - no error checking
    if (fclose( fp ) != 0)  {

        // writing to syslog -> error not reported
        // printf( "Error on closing file '%s'", filename);
        syslog(LOG_ERR, "Error on closing file '%s'", filename);

        closelog();

        return 1;

    }


    // syslog report writing
    syslog(LOG_DEBUG, "Writing %s to %s", text2write, filename);

    closelog();

    return 0;

}
