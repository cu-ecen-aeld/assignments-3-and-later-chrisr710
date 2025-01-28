#!/bin/bash
if [ "$#" != "2" ]; then
	echo "first argument is the full path to the file, including filename. Second argument is the string to write."
	exit 1
fi
filepath="$1"
string_to_write="$2"
if [ ! -d "$(dirname "$filepath")" ]; then
	mkdir -p "$(dirname "$filepath")"
        #echo "made ""(dirname "$filepath")"
        if [ "$?" != "0" ]; then
		echo "Could not create path "$filepath""
		exit 1
	fi
fi
echo "$string_to_write" > "$filepath"
if [ "$?" != "0" ]; then
        echo "Could not write string to  "$filepath""
        exit 1
fi
exit 0


