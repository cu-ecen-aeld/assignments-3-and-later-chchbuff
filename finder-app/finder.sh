#!/bin/sh

# validate runtime arguments
if [ $# -lt 2 ]
then
	echo "ERROR: Invalid Number of Arguments."
	echo "Total number of arguments should be 2."
	echo "The order of the arguments should be:"
	echo "\t1)File Directory Path."
	echo "\t2)String to be searched in the specified directory path."
	exit 1
else
	filesdir=$1
	searchstr=$2
	if [ -d "$filesdir" ]
	then
		echo "$filesdir is a directory"
	else
		echo "$filesdir is not a directory"
		exit 1
	fi
fi

# stores the number of files present in the directory
filecount=`find $filesdir -type f | wc -l`
if [ $? -eq 1 ]; then
	echo "ERROR: running find command to find number of files"
	exit 1
fi

# stores number of matching lines found with the search string in the directory
matchinglinescount=`grep -r $searchstr $filesdir | wc -l`
if [ $? -eq 1 ]; then
	echo "ERROR: running grep command to search string"
	exit 1
fi

echo "The number of files are $filecount and the number of matching lines are $matchinglinescount"
