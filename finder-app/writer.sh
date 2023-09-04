#!/bin/sh

if [ $# -lt 2 ]
then
        echo "ERROR: Invalid Number of Arguments."
        echo "Total number of arguments should be 2."
        echo "The order of the arguments should be:"
        echo "\t1)Full path to a file."
        echo "\t2)Text string to be written to a file."
        exit 1
else
        writefile=$1
        writestr=$2
	filedir=`dirname $writefile`
	filename=`basename $writefile`
        if [ ! -d "$filedir" ]
        then
		mkdir -p $filedir
		if [ $? -eq 1 ]; then
			echo "ERROR:Unable to create directory."
			exit 1
		fi
	fi
	cd $filedir
	touch $filename
       	echo $writestr > $filename
	exit 0
fi
