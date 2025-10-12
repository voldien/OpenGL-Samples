#!/bin/bash
#
# SCRIPT: EntryPoint for handling multiple executable samples
# AUTHOR: Valdemar Lindberg
# DATE:
# REV: 0.1.B
#
# PLATFORM: Linux
#
# PURPOSE: Handle the whole process of generating an AppImage

# set -n
# Uncomment to check script syntax, without execution.
#
# NOTE: Do not forget to put the comment back in or
#
# the shell script will not execute!
# set -x
# Uncomment to debug this shell script
#

ROOT_PROGRAM=$0
SELECTED_SAMPLE_EXEC=$1
shift 1
SELECTED_SAMPLE_EXEC_ARGS="$@"

echo $SELECTED_SAMPLE_EXEC
echo $SELECTED_SAMPLE_EXEC_ARGS

$SELECTED_SAMPLE_EXEC $SELECTED_SAMPLE_EXEC_ARGS
