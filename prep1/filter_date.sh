#!/bin/sh
if [ $# -ne 1 ]; then
	(>&2 echo "Invalid arguement count! Must provide input file as arguement.")
	exit 1
fi
echo "Searching in file '$1'."
DSTRING=$(cat $1 | grep "^Date: ")
echo "Found: '$DSTRING'."

