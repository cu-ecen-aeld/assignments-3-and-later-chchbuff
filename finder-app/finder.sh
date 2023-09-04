#!/bin/sh


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

filecount=`find $filesdir -type f | wc -l`
matchinglinescount=`grep -r $searchstr $filesdir | wc -l`

echo "The number of files are $filecount and the number of matching lines are $matchinglinescount"
exit 0
