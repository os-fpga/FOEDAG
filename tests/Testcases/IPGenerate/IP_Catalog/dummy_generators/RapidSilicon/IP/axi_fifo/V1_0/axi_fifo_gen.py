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
                32,
                64,
                128,
                256,
                512,
                1024
            ],
            "default": "32",
            "type": "int",
            "description": "FIFO Data Width."
        },
        {
            "parameter": "write_fifo_depth",
            "title": "WRITE_FIFO_DEPTH",
            "options": [
                0,
                32,
                512
            ],
            "default": "0",
            "type": "int",
            "description": "FIFO Write Depth."
        },
        {
            "parameter": "read_fifo_depth",
            "title": "READ_FIFO_DEPTH",
            "options": [
                0,
                32,
                512
            ],
            "default": "0",
            "type": "int",
            "description": "FIFO Read Depth."
        },
        {
            "parameter": "aw_user_en",
            "title": "AW_USER_EN",
            "default": "True",
            "type": "bool",
            "description": "FIFO AW-Channel User Enable."
        },
        {
            "parameter": "w_user_en",
            "title": "W_USER_EN",
            "default": "True",
            "type": "bool",
            "description": "FIFO W-Channel User Enable."
        },
        {
            "parameter": "b_user_en",
            "title": "B_USER_EN",
            "default": "True",
            "type": "bool",
            "description": "FIFO B-Channel User Enable."
        },
        {
            "parameter": "ar_user_en",
            "title": "AR_USER_EN",
            "default": "True",
            "type": "bool",
            "description": "FIFO AR-Channel User Enable."
        },
        {
            "parameter": "r_user_en",
            "title": "R_USER_EN",
            "default": "True",
            "type": "bool",
            "description": "FIFO R-Channel User Enable."
        },
        {
            "parameter": "write_fifo_delay",
            "title": "WRITE_FIFO_DELAY",
            "default": "True",
            "type": "bool",
            "description": "FIFO Write Delay."
        },
        {
            "parameter": "read_fifo_delay",
            "title": "READ_FIFO_DELAY",
            "default": "True",
            "type": "bool",
            "description": "FIFO Read Delay."
        },
        {
            "parameter": "addr_width",
            "title": "ADDR_WIDTH",
            "range": [
                1,
                64
            ],
            "type": "int",
            "default": "32",
            "description": "FIFO Address Width."
        },
        {
            "parameter": "id_width",
            "title": "ID_WIDTH",
            "range": [
                1,
                32
            ],
            "type": "int",
            "default": "1",
            "description": "FIFO ID Width."
        },
        {
            "parameter": "aw_user_width",
            "title": "AW_USER_WIDTH",
            "range": [
                1,
                1024
            ],
            "type": "int",
            "default": "1",
            "description": "FIFO AW-Channel User Width.",
            "dependency": "aw_user_en"
        },
        {
            "parameter": "w_user_width",
            "title": "W_USER_WIDTH",
            "range": [
                1,
                1024
            ],
            "type": "int",
            "default": "1",
            "description": "FIFO W-Channel User Width.",
            "dependency": "w_user_en"
        },
        {
            "parameter": "b_user_width",
            "title": "B_USER_WIDTH",
            "range": [
                1,
                1024
            ],
            "type": "int",
            "default": "1",
            "description": "FIFO B-Channel User Width.",
            "dependency": "b_user_en"
        },
        {
            "parameter": "ar_user_width",
            "title": "AR_USER_WIDTH",
            "range": [
                1,
                1024
            ],
            "type": "int",
            "default": "1",
            "description": "FIFO AR-Channel User Width.",
            "dependency": "ar_user_en"
        },
        {
            "parameter": "r_user_width",
            "title": "R_USER_WIDTH",
            "range": [
                1,
                1024
            ],
            "type": "int",
            "default": "1",
            "description": "FIFO R-Channel User Width.",
            "dependency": "r_user_en"
        }
    ],
    "build": "False",
    "build_dir": "./",
    "build_name": "axi_fifo_wrapper",
    "json": "None",
    "json_template": "True"
}



print( json.dumps(obj) )
    
