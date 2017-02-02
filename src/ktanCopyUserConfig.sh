#!/bin/sh
#
# shell script to automatically copy configuration files for ktan.
#
# The script takes 1 argument which is the name of the user
#
# It assumes that the configuration files of the user have the same name as the one 
# used by ktan but with the .user extension.
# For example ktan.file.base.julia instead of ktan.file.base
#
# The shell script is doing: cp ktan.file.base.julia ktan.file.base
# The original ktan.file.base is overwritten.

while getopts tr name
do
    case $name in
#	t)
#	    tflag=1;;
	*)  echo Usage: $0 [-t][-d] session_list file_ext>&2
            exit 2;;
    esac
done
shift $((OPTIND-1)) ## remove opt from $# 

#check if 1 argument
if [ "$#" != "1" ]
then
    echo "$0 needs one argument which is the name of the user." >&2
    echo "$0 copies the user specific configuration file that will be used by ktan." >&2
    echo "For example, if user is julia, the program copies ktan.file.base.julia to ktan.file.base" >&2
    exit 1
fi

user=$1

for file in `echo "ktan.file.base ktan.max.recording.time ktan.oscilloscope.group.channels 
ktan.recording.channels"`
do
	if [ -e "$file.$user" ] 
	then	
		echo "copy $file.$user to $file"
		cp $file.$user $file
	else
		echo "$file.$user is missing"
	fi		
done

