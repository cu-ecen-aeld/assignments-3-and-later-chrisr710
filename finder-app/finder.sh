#!/bin/sh
filesdir="$1"
seearchstr="$2"
if [ "$#" -ne "2" ]; then
	echo "requires 2 arguments: files directory and search string"
	exit 1
fi

if [ ! -d "$filesdir" ]; then
	echo "Directory ""$filesdir"" does not exist!"
	exit 1
fi


filelist="$(find "$filesdir" -type f)"
x=0
y="0"
for f in $filelist
	do
        let x=x+1
	count_for_file="$(grep -c "$searchstr" $f)"
        let y=y+"$count_for_file"
        done
echo "The number of files are $x and the number of matching lines are $y"
