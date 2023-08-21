Following JSON file version are being used:

* "gold_hier.json": This is the original json file outputted by the analyze tool that was analyzing the gpio.sv file and its submodules.
* "gold_hier_v2.json": This is the v2 of the original json file with additional nodes added such as "cpus", "cpus-cluster, "memory", etc.
* "gold_hier_v3.json": This is the v3 of the original json file but this JSON file contails all the nodes from the gemini sdt available here https://github.com/RapidSilicon/zephyr-rapidsi-dev2/blob/c9c26d648db1a698d48083f45ab856e9a4726be7/dts/riscv/rapidsi/rapidsi_gemini.dtsi. A new tcl script will be needed to parse this JSON file and output a SDT from this JSON file. 
