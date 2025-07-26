#/bin/sh

#
# finder.sh
#
# Arguments: <path> <searchstring>
#


# test for all parameters <path> and <searchstring>
if [ $# -ne 2 ]
then
    echo "Error: script requires two arguments: <path> <searchstring>"
    exit 1
fi

# assign parameters 
FILEDIR="$1"
SEARCHSTR="$2"

# test for <filesdir> is link to directory
if [ ! -d "$FILEDIR" ] 
then
    echo "Error: '$FILEDIR' is not a valid directory"
    exit 1
fi

# get the amount of files
X=$(find "$FILEDIR" -type f | wc -l)

# get the amaount of matching strings
Y=$(grep -r "$SEARCHSTR" "$FILEDIR" | wc -l)

echo "The number of files are $X and the number of matching lines are $Y"

exit 0
