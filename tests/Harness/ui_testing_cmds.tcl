#  Sleep/uniqkey functions taken from https://wiki.tcl-lang.org/page/sleep
# uniqkey is a helper for sleep only
proc uniqkey { } {
    set key   [ expr { pow(2,31) + [ clock clicks ] } ]
    set key   [ string range $key end-8 end-3 ]
    set key   [ clock seconds ]$key
    return $key
}
# emulates a c style sleep so you don't have to deal with tcl after commands
proc sleep { ms } {
    set uniq [ uniqkey ]
    set ::__sleep__tmp__$uniq 0
    after $ms set ::__sleep__tmp__$uniq 1
    vwait ::__sleep__tmp__$uniq
    unset ::__sleep__tmp__$uniq
}

# Eval cmd, if it fires an error, delay and then retry the command
proc repeatOnError { cmd {retries 5} {delay 100} } {
    set retry 1
    set count 0
    while {$retry && $count < $retries} {
        if {[catch {eval $cmd} result]} {
            sleep $delay
            set retry 1
            incr count 1
        } else {
            set retry 0
        }
    }

    if { $retry == 1 } {
        # failed on last attempt
        throw "reaptOnErrorLimitReached" "repeatOnError: Command '$cmd' failed $retries times. Retry limit reached."
    }

    # remove whitespace/newlines off end of result
    set result [string trim $result]

    return $result
}

# fire qt_findWidgets with given args. If no widget is found, the search is re-attempted after a delay
proc waitForWidget {args} {
    set cmd "qt_findWidgets $args"
    return [repeatOnError $cmd]
}

# fire qt_findWidgets with given args. If no widget is found, the search is re-attempted after a delay
# retry count and delay time are customizable
proc waitForWidgetEx { retries delay args } {
    set cmd "qt_findWidgets $args"
    return [repeatOnError $cmd $retries $delay]
}
