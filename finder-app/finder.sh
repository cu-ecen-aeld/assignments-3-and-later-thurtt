#!/bin/sh

filesdir=$1
searchstr=$2

optsok=0

# Validate our command line
if [ -z $filesdir ]; then
    echo "Error: The search directory was not provided"
    optsok=1
fi

if [ -z $searchstr ]; then
    echo "Error: No search string was provided"
    optsok=1
fi

if [ ! -d $filesdir ]; then
    echo "Error: Specified search directory: $filesdir is invalid. Please make sure"
    echo "the directory exists and is readable."
    optsok=1
fi

if [ $optsok -eq 1 ]; then
    echo "-------------------------------------------------------------------"
    echo ""
    echo "Finder usage:"
    echo "\$ ./finder.sh <filesdir> <searchstr>"
    echo ""
    echo "Where:"
    echo "<filesdir> is the directory to search"
    echo "<searchstr> is the string to find inside the search directory"
    echo ""
    exit 1
fi


filecount=`find $filesdir//. ! -name . -print 2>/dev/null | grep -c //`
search_match=`grep -R $searchstr $filesdir 2>/dev/null | wc -l`

echo "The number of files are $filecount and the number of matching lines are $search_match"
