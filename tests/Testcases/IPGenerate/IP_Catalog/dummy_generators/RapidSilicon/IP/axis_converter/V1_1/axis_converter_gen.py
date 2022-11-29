#!/usr/bin/env python3
import json

# The following is the captured output of `python3 axis_converter_gen.py --json-template`
# Which is what the IPCatalog calls when first reading in IPs
# Foedag currently doesn't have the dependencies(litex/migen) to run that command
# on the fly so this file is a temporary solution

obj = {
	"parameters": [
        {
            "parameter": "core_in_width",
            "default": "128"
	    },
        {
            "parameter": "core_out_width",
            "default": "64"
        },
        {
            "parameter": "core_user_width",
            "default": "0"
	    },
        {
            "parameter": "core_reverse",
            "default": "False"
	    }
    ],
	"build": "False",
	"build_dir": "build",
	"build_name": "axis_converter_128b_to_64b",
	"json": "None",
	"json_template": "True"
}

print( json.dumps(obj) )
