gui_start
create_design log_header

# create an empty report before we run the analyze the step
set fp [open "log_header/fake.rpt" w]
close $fp

analyze
help

set fp [open "log_header/analysis.rpt" r]
set file_data [read $fp]
close $fp

# Verify Copyright is printed
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

# Verify foedag.log has INFO: ANL: abbreviation in front of analyze messages
set found [regexp "\nINFO: ANL: Design log_header is analyzed" $file_data]
if { !$found } { puts "ERROR: foedag.log's Analyze messages don't have INFO: ANL: in front"; exit 1 }

# Verify help message doesn't have an abbreviation in front of it
set found [regexp "\n=-= FOEDAG HELP   =-=" $file_data]
if { !$found } { puts "ERROR: foedag.log's help message has an abbreviation before it or wasn't printed"; exit 1 }

# Verify that the fake.rpt file now has copyright info because we now dynamically add the header to all *.rpt's in a project folder
set fp [open "log_header/fake.rpt" r]
set file_data [read $fp]
close $fp
set found [regexp "Copyright 20\\d\\d The Foedag team" $file_data]
if { !$found } { puts "ERROR: fake.rpt is missing header info"; exit 1 }

# passed
exit
