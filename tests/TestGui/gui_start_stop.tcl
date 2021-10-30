set errorInfo ""

catch {
gui_start ; puts "GUI START" ; flush stdout

after 500  "gui_stop; puts \"GUI STOP\"; flush stdout"
after 1000 "gui_start; puts \"GUI START\"; flush stdout"
after 1500 "gui_stop; puts \"GUI STOP\"; flush stdout"

after 2000 "puts \"GUI EXIT\" ; flush stdout; set CONT 0"

set CONT 1 
while {$CONT} {
    set a 0
    after 100 set a 1
    vwait a
}

puts "Tcl Exit" ; flush stdout
}  

if {$errorInfo != ""} {
 puts $errorInfo
 exit 1
}
tcl_exit

