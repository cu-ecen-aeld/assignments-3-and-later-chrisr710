#!/bin/sh
# Tester script for assignment 1 and assignment 2
# Author: Siddhant Jajoo

set -e
set -u

NUMFILES=10
WRITESTR=AELD_IS_FUN
WRITEDIR=/tmp/aeld-data
#username=$(cat conf/username.txt)
running_remote=0

[ "$(pwd)" != "/home/school/school-repository/finder-app" ] && running_remote=1
if [ "$running_remote" -eq "0" ]
	then
		username=$(cat conf/username.txt)
	else
		username=$(cat /etc/finder-app/conf/username.txt)
fi
	

if [ $# -lt 3 ]
then
	echo "Using default value ${WRITESTR} for string to write"
	if [ $# -lt 1 ]
	then
		echo "Using default value ${NUMFILES} for number of files to write"
	else
		NUMFILES=$1
	fi	
else
	NUMFILES=$1
	WRITESTR=$2
	WRITEDIR=/tmp/aeld-data/$3
fi

MATCHSTR="The number of files are ${NUMFILES} and the number of matching lines are ${NUMFILES}"

echo "Writing ${NUMFILES} files containing string ${WRITESTR} to ${WRITEDIR}"

rm -rf "${WRITEDIR}"

# create $WRITEDIR if not assignment1
if [ $running_remote -eq "0" ]
	then
	assignment=`cat ../conf/assignment.txt`
	else
	assignment=`cat /etc/finder-app/conf/assignment.txt`
fi

if [ $assignment != 'assignment1' ]
then
	mkdir -p "$WRITEDIR"

	#The WRITEDIR is in quotes because if the directory path consists of spaces, then variable substitution will consider it as multiple argument.
	#The quotes signify that the entire string in WRITEDIR is a single string.
	#This issue can also be resolved by using double square brackets i.e [[ ]] instead of using quotes.
	if [ -d "$WRITEDIR" ]
	then
		echo "$WRITEDIR created"
	else
		exit 1
	fi
fi
#echo "Removing the old writer utility and compiling as a native application"
#make clean
#make

WRITER_PATH="./writer"
[ ! -e "$WRITER_PATH" ] && WRITER_PATH="$(which writer)"
echo "WRITER_PATH =""$WRITER_PATH"
for i in $( seq 1 $NUMFILES)
do
	$WRITER_PATH "$WRITEDIR/${username}$i.txt" "$WRITESTR"
done

FINDER_PATH="./finder.sh"
[ ! -e "$FINDER_PATH" ] && FINDER_PATH="$(which finder.sh)"
OUTPUTSTRING=$(./finder.sh "$WRITEDIR" "$WRITESTR") 
echo "$OUTPUTSTRING" > /tmp/assignment-4-result.txt
# remove temporary directories
rm -rf /tmp/aeld-data

set +e
echo ${OUTPUTSTRING} | grep "${MATCHSTR}"
if [ $? -eq 0 ]; then
	echo "success"
	exit 0
else
	echo "failed: expected  ${MATCHSTR} in ${OUTPUTSTRING} but instead found"
	exit 1
fi
