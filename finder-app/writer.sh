#!/bin/sh

# Map arguments to variables

writefile=$1
writestr=$2

# Verify both arguments are present

if [ $# -ne 2 ]; then
	echo "Error.  Must provide two arguments"
	exit 1
fi

#Get the directory from the file path
directory=$(dirname "$writefile")

# I directory doesn't exist, then create it
if [ ! -d "$directory" ]; then
	mkdir -p "$directory"
	if [ $? -ne 0 ]; then
		echo "Error.  Unable to create directory."
		exit 1
	fi
fi

# Write to file

echo "$writestr" > "$writefile"
if [ $? -ne 0 ]; then
	echo "Error.  Could not create file '$writefile'."
	exit 1
fi

echo "File '$writefile' created."