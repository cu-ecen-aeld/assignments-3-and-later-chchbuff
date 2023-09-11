#!/bin/sh

# validate runtime arguments
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
	# extract file directory and file name
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

	echo $writestr > $writefile
	if [ $? -eq 1 ]; then
                echo "ERROR: writing string to $filename"
                exit 1
	else
		echo "Written string to $filename successfully"
        fi
fi
