#!/usr/bin/env python3
import json

# The following is the captured output of `python3 axis_converter_gen.py --json-template`
# Which is what the IPCatalog calls when first reading in IPs
# Foedag currently doesn't have the dependencies(litex/migen) to run that command
# on the fly so this file is a temporary solution

obj = {
    "core_in_width": 128,
    "core_out_width": 64,
    "core_user_width": 0,
    "core_reverse": False,
    "build": False,
    "build_dir": "build",
    "build_name": "axis_converter_128b_to_64b",
    "json": None,
    "json_template": True
}

print( json.dumps(obj) )
