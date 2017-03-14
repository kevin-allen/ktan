#!/bin/sh
#
# shell script to start and stop ktan and positrack
#
# The script takes 1 argument which is the recording time
#
# Assumes that ktan and positrack are using the configuration files to set recording channels, filebase, etc.
#
# This will start ktan, wait 2 sec,  start positrack, wait rec time, stop positrack, stop ktan
#
# After the trial, the synchronization between ktan and positrack will be tested using relectro R package.
Zflag=
while getopts Z: name
do
    case $name in
	f)
	    Zflag=1;;
	*)  echo "Usage: $0 recTimeMinutes" >&2
            exit 2;;
    esac
done
shift $((OPTIND-1)) ## remove opt from $# 

#check if 1 argument
if [ "$#" != "1" ]
then
    echo "$0 needs one argument which is the recording time in minutes." >&2
    echo "Usage: $0 [-f fileBase] 20"
    exit 1
fi

recTime=$1

echo "recording for $recTime minutes"
exit
ktanStartStop start_rec
sleep 2s
positrackStartStop start_tracking
sleep "$recTime"m
positrackStartStop stop_tracking
sleep 2s
ktanStartStop stop_rec
sleep 1s

# get latest .dat and .positrack in home directory
cd ~
positrackFile=`ls -t *.positrack | head -n 1`
datFile=`ls -t *.dat | head -n 1`
nChannels=`cat ktan.recording.channels | wc -l`
ttlChannel=$((nChannels-1))
echo "positrack file: $positrackFile"
echo "dat file: $datFile"
echo "nChannels: $nChannels"
echo "ttlChannel: $ttlChannel"

# check synchronization with using relectro R package
echo "checking synchronization with relectro R package"
