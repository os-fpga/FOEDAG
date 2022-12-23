set fp [open "foedag.log" r]
set file_data [read $fp]
close $fp

# Note: this test must be launched via --batch to verify this scenario.
# This test ensures that copyright info is accesible at launch which
# inherently ensures that --batch sets it's dataPath info in the Context
# instance which wasn't being done prior to this check-in

# Verify Copyright is printed when launching with --batch
set found [regexp "Copyright 20\\d\\d The Foedag team" $file_data]
if { !$found } {
    puts "ERROR: foedag.log missing copyright info after starting with --batch";
    exit 1
}

# passed
exit
