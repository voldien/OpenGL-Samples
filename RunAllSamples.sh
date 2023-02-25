#!/bin/bash

PATHDIR=$1

for FILE in $PATHDIR/*; do # Whitespace-safe but not recursive.
	echo "Running $FILE"
	if [ -f "${FILE}" ]; then
		$FILE --time 10 --fullscreen --vsync
	fi
done
