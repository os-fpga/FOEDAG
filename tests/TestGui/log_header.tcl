gui_start

create_design log_header

analyze

set fp [open "log_header/analysis.rpt" r]
set file_data [read $fp]
close $fp

# Verify Copyrighty is printed
set found [regexp "Copyright 20\\d\\d The Foedag team" $file_data]
# one line for --replay
if { !$found } { puts "ERROR: analysis.rpt is missing copyright info"; exit 1 }

# Verify Version is printed
set found [regexp "Version.*: " $file_data]
if { !$found } { puts "ERROR: analysis.rpt is missing version info"; exit 1 }

# Verify Build Date is printed
set found [regexp "Built.*: " $file_data]
if { !$found } { puts "ERROR: analysis.rpt is missing version info"; exit 1 }

# Verify Build Date is printed
set found [regexp "Built type.*: " $file_data]
if { !$found } { puts "ERROR: analysis.rpt is missing build type info"; exit 1 }

# Verify Log Time is printed
set found [regexp "Log Time.*: " $file_data]
if { !$found } { puts "ERROR: analysis.rpt is missing log time info"; exit 1 }

# Verify original content is printed
set found [regexp "Dummy log for analysis.rpt" $file_data]
if { !$found } { puts "ERROR: analysis.rpt is missing original contents info"; exit 1 }

# Verify foedag_cmd.tcl prints header info w/ tcl comments prefixed
set fp [open "foedag_cmd.tcl" r]
set file_data [read $fp]
close $fp
set found [regexp "# Copyright 20\\d\\d The Foedag team" $file_data]
if { !$found } { puts "ERROR: foedag_cmd.tcl has an uncommented header"; exit 1 }

# Verify foedag.log prints header info w/ tcl comments prefixed
set fp [open "foedag.log" r]
set file_data [read $fp]
close $fp
set found [regexp "^Copyright 20\\d\\d The Foedag team" $file_data]
if { !$found } { puts "ERROR: foedag.log's header is commented which shouldn't have occured"; exit 1 }

# passed
exit
