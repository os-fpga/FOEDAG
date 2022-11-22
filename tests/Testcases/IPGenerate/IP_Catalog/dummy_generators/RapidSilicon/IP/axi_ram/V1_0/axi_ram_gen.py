#!/usr/bin/env python3
import json

# The following is simulated output of `python3 <some_generator>_gen.py --json-template`
# which is what the IPCatalog calls when first reading in IPs
# Foedag currently doesn't have the dependencies(litex/migen) to run that command
# on the fly so this file is a temporary solution

obj = {
    "parameters": [
        {
            "parameter": "data_width",
            "title": "DATA_WIDTH",
            "options": [
                8,
                16,
                32,
                64
            ],
            "default": "32",
            "type": "int",
            "description": "RAM Data Width."
        },
        {
            "parameter": "pip_out",
            "title": "PIP_OUT",
            "default": "False",
            "type": "bool",
            "description": "RAM Pipeline Output."
        },
        {
            "parameter": "addr_width",
            "title": "ADDR_WIDTH",
            "range": [
                8,
                16
            ],
            "type": "int",
            "default": "16",
            "description": "RAM Address Width."
        },
        {
            "parameter": "id_width",
            "title": "ID_WIDTH",
            "range": [
                1,
                8
            ],
            "type": "int",
            "default": "8",
            "description": "RAM ID Width."
        },
        {
            "parameter": "file_path",
            "title": "FILE_PATH",
            "default": "./",
            "type": "filepath",
            "description": "File Path for memory initialization file"
        }
    ],
    "build": "False",
    "build_dir": "./",
    "build_name": "axi_ram_wrapper",
    "json": "None",
    "json_template": "True"
}


print( json.dumps(obj) )
    
