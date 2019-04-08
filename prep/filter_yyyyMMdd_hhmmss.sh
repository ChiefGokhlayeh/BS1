#!/bin/bash

if [ $# -ne 1 ]; then (>&2 echo "Error: Missing file argument!"); exit 1; fi

DSTRING=$(cat $1 | grep "^Date: ")

echo "$DSTRING"

#Example 'Date: Fri, 9 Mar 2018 13:30:40 +0000'
IFS=': ' read -r -a split <<< $DSTRING
day=${split[2]}
monthText=${split[3]}
year=${split[4]}
hour=${split[5]}
minute=${split[6]}
second=${split[7]}

case $monthText in
	Jan)
		month=1
		;;
	Feb)
		month=2
		;;
	Mar)
		month=3
		;;
	Apr)
		month=4
		;;
	May)
		month=5
		;;
	Jun)
		month=6
		;;
	Jul)
		month=7
		;;
	Aug)
		month=8
		;;
	Sep)
		month=9
		;;
	Oct)
		month=10
		;;
	Nov)
		month=11
		;;
	Dec)
		month=12
		;;
	*)
		(>2& echo "Error: Unexpected month string!")
		exit 1
esac

printf "%04d-%02d-%02d %02d:%02d:%02d\n" $year $month $day $hour $minute $second

