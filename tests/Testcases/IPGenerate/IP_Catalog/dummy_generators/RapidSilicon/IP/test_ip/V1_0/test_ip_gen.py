#!/usr/bin/env python3
import json

# The following is simulated output of `python3 <some_generator>_gen.py --json-template`
# which is what the IPCatalog calls when first reading in IPs
# Foedag currently doesn't have the dependencies(litex/migen) to run that command
# on the fly so this file is a temporary solution

# This particular generator returns json in the new format called out in EDA-357

obj = {
    "parameters": [
        {
            "parameter" : "data_width",
            "title" : "Data Width",
            "options" : [8,16,32,64],
            "default" : "32",
            "type" : "int"
        },
        {
            "parameter" : "write_width",
            "title" : "Write Width",
            "range" : [1,4608],
            "default" : "1",
            "type" : "int"
        },
        {
            "parameter" : "C_IN",
            "title" : "Carry In (C_IN)",
            "type" : "bool",
            "default" : "1"
        },
        {
            "parameter" : "AWUSER_WIDTH",
            "title" : "AWUSER_WIDTH",
            "options" : [0,1024],
            "default" : "0",
            "type" : "int",
            "dependency" : "bool_for_dep"
        },
        {
            "parameter" : "init_file_path",
            "title" : "Init File Path",
            "default" : "/some/path",
            "type" : "filepath"
        },
        {
          "parameter" : "str_param",
          "title" : "String Parameter",
          "default" : "some string",
          "type" : "string"
        },
        {
            "parameter" : "int_range_ex",
            "title" : "Int Range Example",
            "range" : [1,10],
            "default" : "1",
            "type" : "int"
        },
        {
            "parameter" : "float_range_ex",
            "title" : "Float Range Example",
            "range" : [1.0,4608.0],
            "default" : "1.1",
            "type" : "float"
        },
        {
            "parameter" : "float_options_ex",
            "title" : "Float Options Example",
            "options" : ["1.1","2.2","3.3","4.4","5.5"],
            "default" : "1.1",
            "type" : "float"
        },
        {
            "parameter" : "str_array",
            "title" : "String Array Example",
            "options" : ["val1", "val2", "val3"],
            "default" : "val2",
            "type" : "string"
        },
        {
            "parameter" : "multi_dep",
            "title" : "Multiple Dependency Test",
            "default" : "this value depends on boolParam1 and boolParam2",
            "type" : "int",
            "dependency" : ["boolParam1", "boolParam2"]
        },
        {
            "parameter" : "bool_for_dep",
            "title" : "bool for dep test",
            "type" : "bool",
            "default" : "0"
        },
        {
            "parameter" : "field_with_dep",
            "title" : "depends on above checkbox",
            "default" : "27",
            "type" : "int",
            "dependency" : "bool_for_dep"
        },
        {
            "parameter" : "bad_def_val",
            "title" : "Non string default value error test",
            "default" : "this value depends on boolParam1 and boolParam2",
            "type" : "int",
            "description" : "tooltip test"
        },
        {
            "parameter" : "no_title_param",
            "default" : "this has no title set",
            "type" : "string",
            "description" : "this has no title set"
        },
        {
            "parameter" : "spaces_field",
            "title" : "field with spaces",
            "default" : "this field has spaces",
            "type" : "string",
            "description" : "this field has spaces"
        },
        {
            "parameter": "str_ex",
            "title": "String field",
            "options": [
                1,
                1024
            ],
            "type": "string",
            "default": "1",
            "description": "string test"
        }

        
    ],
    "build_dir": "build",
    "build_name": "new_ip_wrapper",
    "build": False,
    "json": None,
    "json_template": False
}

print( json.dumps(obj) )
    
