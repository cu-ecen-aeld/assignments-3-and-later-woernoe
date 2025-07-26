#!/bin/sh

# Writer.sh
#
# Arguments: <filename> <write string>
#

# check two arguments
if [ "$#" -ne 2 ]
then
    echo "Expecting two arguemnts <path to file> <text string>" 
    exit 1
fi

# set variables
WRITEFILE="$1"
WRITESTR="$2"

# test if writefile links to a file
if [ -d "$WRITEFILE" ]
then
    echo "Error: '$WRITEFILE' is a directory, not a file"
    exit 1
fi

# test for directory
DIRECTORY=$(dirname "$WRITEFILE")

#create directory if it does not exist
mkdir -p "$DIRECTORY" 2>/dev/null
if [ $? -ne 0 ]
then
    echo "Error: could not create directory path '$DIRECTORY'"!
    exit 1
fi

# write file
echo "$WRITESTR" > "$WRITEFILE" 2>/dev/null
if [ $? -ne 0 ]
then
    echo "Error: writing to file '$WRITEFILE'"
    exit 1
fi

exit 0
