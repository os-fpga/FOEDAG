#!/usr/bin/env python3
import json

# The following is simulated output of `python3 <some_generator>_gen.py --json-template`
# which is what the IPCatalog calls when first reading in IPs
# Foedag currently doesn't have the dependencies(litex/migen) to run that command
# on the fly so this file is a temporary solution

obj = {
    "build_dir": "build",
    "build_name": "no_params_wrapper",
    "build": False,
    "json": None,
    "json_template": False
}

print( json.dumps(obj) )
    
