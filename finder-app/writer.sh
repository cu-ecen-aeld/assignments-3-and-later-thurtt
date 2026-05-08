#!/bin/sh

writefile=$1
writestr=$2

optsok=0

# Validate our command line
if [ -z $writefile ]; then
    echo "Error: The writefile was not provided"
    optsok=1
fi

if [ -z $writestr ]; then
    echo "Error: The string to write was not provided"
    optsok=1
fi

if [ $optsok -eq 1 ]; then
    echo "-------------------------------------------------------"
    echo ""
    echo "Writer usage:"
    echo "\$ ./writer.sh <writefile> <writestr>"
    echo ""
    echo "Where:"
    echo "<writefile> is the full path to the file to create"
    echo "<writestr> is the string to write to the file"
    echo ""
    exit 1
fi

dirpath=`dirname $writefile`
if [ ! -d $dirpath ]; then
    echo "Creating $dirpath directory..."
    mkdir -p $dirpath
fi

echo "Writing '$writestr' to $writefile"
echo $writestr > $writefile

if [ $? -ne 0 ]; then
    echo "Error: Could not write string to $writefile"
    exit 1
fi
