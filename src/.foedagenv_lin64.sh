#!/bin/bash

RETURN_PATH=`pwd`
SCRIPT_PATH=`dirname $BASH_SOURCE`
FOEDAG_PATH=`( cd "$SCRIPT_PATH" && pwd )`

if [ -n "${LD_LIBRARY_PATH}" ]; then
	export LD_LIBRARY_PATH=$FOEDAG_PATH/lib64/foedag/lib:$FOEDAG_PATH/lib64/openssl:$FOEDAG_PATH/lib64/tbb:$FOEDAG_PATH/lib/tbb:$FOEDAG_PATH/lib64:$FOEDAG_PATH/lib:$FOEDAG_PATH/lib/foedag/lib:$FOEDAG_PATH/bin/gtkwave/lib:$FOEDAG_PATH/external_libs/qt/lib:$FOEDAG_PATH/external_libs/lib:$LD_LIBRARY_PATH
else
	export LD_LIBRARY_PATH=$FOEDAG_PATH/lib64/foedag/lib:$FOEDAG_PATH/lib64/openssl:$FOEDAG_PATH/lib64/tbb:$FOEDAG_PATH/lib/tbb:$FOEDAG_PATH/lib64:$FOEDAG_PATH/lib:$FOEDAG_PATH/lib/foedag/lib:$FOEDAG_PATH/bin/gtkwave/lib:$FOEDAG_PATH/external_libs/qt/lib:$FOEDAG_PATH/external_libs/lib
fi

if [ -n "${TCL_LIBRARY}" ]; then
	export TCL_LIBRARY=$FOEDAG_PATH/share/foedag/tcl8.6.12/library/:$TCL_LIBRARY
else
	export TCL_LIBRARY=$FOEDAG_PATH/share/foedag/tcl8.6.12/library/
fi

if [ -n "${PYTHONPATH}" ]; then
	export PYTHONPATH=$PYTHONPATH/$FOEDAG_PATH/share/foedag/IP_Catalog/:$PYTHONPATH
else
	export PYTHONPATH=$FOEDAG_PATH/share/foedag/IP_Catalog/
fi
[[ -f "$FOEDAG_PATH/bin/HDL_simulator/setup_sim" ]] && source $FOEDAG_PATH/bin/HDL_simulator/setup_sim

# uncomment to open the debug
#export QT_DEBUG_PLUGINS=1
#export QT_QPA_PLATFORM=xcb
#export QT_QPA_PLATFORM_PLUGIN_PATH=$FOEDAG_PATH/external_libs/qt/plugins
#export QT_PLUGIN_PATH=$FOEDAG_PATH/external_libs/qt/plugins
