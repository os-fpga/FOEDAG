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
                64,
                128,
                256
            ],
            "default": "32",
            "type": "int",
            "description": "DMA Data Width."
        },
        {
            "parameter": "enable_unaligned",
            "title": "ENABLE_UNALIGNED",
            "default": "True",
            "type": "bool",
            "description": "Support for unaligned transfers."
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
            "description": "DMA Address Width."
        },
        {
            "parameter": "id_width",
            "title": "ID_WIDTH",
            "range": [
                1,
                32
            ],
            "type": "int",
            "default": "8",
            "description": "DMA ID Width."
        },
        {
            "parameter": "axi_max_burst_len",
            "title": "AXI_MAX_BURST_LEN",
            "range": [
                1,
                256
            ],
            "type": "int",
            "default": "16",
            "description": "DMA AXI burst length."
        },
        {
            "parameter": "len_width",
            "title": "LEN_WIDTH",
            "range": [
                1,
                20
            ],
            "type": "int",
            "default": "20",
            "description": "DMA AXI Width of length field."
        },
        {
            "parameter": "tag_width",
            "title": "TAG_WIDTH",
            "range": [
                1,
                8
            ],
            "type": "int",
            "default": "8",
            "description": "DMA Width of tag field."
        }
    ],
    "build": "False",
    "build_dir": "./",
    "build_name": "axi_cdma_wrapper",
    "json": "None",
    "json_template": "True"
}



print( json.dumps(obj) )
    
