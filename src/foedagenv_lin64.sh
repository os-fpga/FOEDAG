#!/bin/bash

RETURN_PATH=`pwd`
SCRIPT_PATH=`dirname $BASH_SOURCE`
FOEDAG_PATH=`( cd "$SCRIPT_PATH" && pwd )`

if [ -n "${PATH}" ]; then
	export PATH=$FOEDAG_PATH/bin:$PATH
else
	export PATH=$FOEDAG_PATH/bin
fi
