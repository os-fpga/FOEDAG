# SDTGen
RS Internal Repository for System Device Tree Generator (SDTGen)

For running the tcl script, use the following command:
* "tclsh sdtgen.tcl -o output.sdt -f"

# Missing information from JSON file:

List of information that is missing in the JSON file for the sdt are mentioned below. Please refer to filer **“gold_hier_v3.json” in folder “./JSON_Files/GPIO_Yosis_Ver_Example”**, to see what values correspond to the SDT node properties keys mentioned below.

SDT (system device tree) nodes required in the JSON file (minimum):

    1. CPUS node (for ACPU).
       Node properties required in the CPU node:
    • "#address-cells"
    • "#size-cells"
    • "timebase-frequency"
    • "cpu_insts" (list)
       Node properties required inside each cpu_inst (list):
    • "id"
    • "device_type"
    • "sub_device_type"
    • "reg"
    • "reg_address"
    • "reg_size"
    • "status"
    • "compatible"
    • "riscv,isa”
    • "mmu-type"	
    • "clock-frequency"
    • "i-cache-line-size"
    • "d-cache-line-size",
    • "interrupt-controller"
      Node properties required inside each interrupt-controller:
                • "id"
                • "phandle"
                • "compatible"
                • "#address-cells"
                • "#interrupt-cells"
                • "interrupt-controller"

    2. CPUS-CLUSTER node (for BCPU as per the sdt file present in the prev sdt repo).
       Node properties required in the CPUS-CLUSTER node:
        ◦ "#address-cells"
        ◦ "#size-cells"
        ◦ "compatible"
        ◦ "cpu_cluster_insts" (list)
          Node properties required inside each cpu_cluster_insts (list):
            ▪ "id"
            ▪ "device_type"
            ▪ "sub_device_type"
            ▪ "reg"
            ▪ "reg_address"
            ▪ "reg_size"
            ▪ "status"
            ▪ "compatible"
            ▪ "riscv,isa"
    3. MEMORY node.
       Node properties required in the MEMORY node:
        ◦ “memory_insts” (list)
          Node properties required inside each memory_insts (list):
            ▪ "id"
            ▪ "device_type"
            ▪ "sub_device_type"
            ▪ "compatible"
            ▪ "reg"
            ▪ "reg_address"
            ▪ "reg_size" 
    4. SOC node.
       Node properties required in the SOC node:
    • "#address-cells"
    • "#size-cells"
    • "compatible"
    • "ranges"
    • "soc_subsystems" (list)
      Each soc subsystem has properties depending upon the type of subsystem in the list. The value corresponding to the key “subsystem” stores the type of subsystem in the current list object.
      
      Currently we have following types of soc subsystems available for the SDT generation using our TCL APIs:
        ◦ "interrupt-controller"
          Node properties for interrupt controller soc subsystem:
            ▪ "subsystem"
            ▪ "id"
            ▪ "phandle"
            ▪ "compatible"
            ▪ "#address-cells"
            ▪ "#interrupt-cells"
            ▪ "interrupt-controller"
            ▪ "reg"
            ▪ "reg_address"
            ▪ "reg_size"
            ▪ "reg-names"
            ▪ "riscv,max-priority"
            ▪ "riscv,ndev"
            ▪ "interrupts-extended"
        ◦ "uart"
          Node properties for uart soc subsystem:
            ▪ "subsystem"
            ▪ "id"
            ▪ "phandle"
            ▪ "compatible"
            ▪ "interrupts"
            ▪ "interrupt-parent"
            ▪ "clock-frequency"
            ▪ "reg"
            ▪ "reg_address"
            ▪ "reg_size"
            ▪ "reg-shift"
            ▪ "status"
        ◦ "gpio"
          Node properties for gpio soc subsystem:
            ▪ "subsystem"
            ▪ "id"
            ▪ "phandle"
            ▪ "compatible"
            ▪ "interrupts"
            ▪ "interrupt-parent"
            ▪ "gpio-controller"
            ▪ "ngpios"
            ▪ "#gpio-cells"
            ▪ "reg"
            ▪ "reg_address"
            ▪ "reg_size"
            ▪ "status"
        ◦ "syscon"
          Node properties for syscon soc subsystem:
            ▪ "subsystem"
            ▪ "id"
            ▪ "phandle"
            ▪ "compatible"
            ▪ "reg"
            ▪ "reg_address"
            ▪ "reg_size"
            ▪ "status"
        ◦ "timer"
          Node properties for syscon soc subsystem:
            ▪ "subsystem"
            ▪ "id"
            ▪ "phandle"
            ▪ "compatible"
            ▪ "interrupts-extended"
            ▪ "reg"
            ▪ "reg_address"
            ▪ "reg_size"
            ▪ "status"


