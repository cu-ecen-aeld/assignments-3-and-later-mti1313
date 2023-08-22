#!/bin/sh

# Map arguments to variables

filesdir=$1
searchstr=$2

# Verify both arguments are present

if [ $# -ne 2 ]; then
		echo "Error: Must provide two arguments."
		exit 1
fi

# Check to see if 'filesdir' is a directory

if [ ! -d "$filesdir" ]; then
	echo "'$filesdir' is not a directory."
	exit 1
fi

# Count the total number of files in the given path
files=$(find "$filesdir" -type f | wc -l)

# Count search string matches in the given path
matches=$(grep -r "$searchstr" "$filesdir" | wc -l)

# Print the results
echo "The number of files are $files and the number of matching lines are $matches."
