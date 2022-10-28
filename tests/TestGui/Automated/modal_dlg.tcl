# Load our helper commands
source tests/Harness/ui_testing_cmds.tcl

# Currently the gui keeps the tcl script from erroring out so we have to try catch the entire test, should explore a better solution for this
try {
    # This test demonstrates how to automate a test when a modal dialog.
    # Normally a modal dialog blocks the execution of our tcl scripts.
    # To get around this, we use qt_queue_cmds which will execute the passed
    # commands via a lambda after the modal dialog has started blocking

    # You need to collect all the commands you want to run while the modal dialog is up.
    # Timing doesn't seem to be an issue on the c/qt side so waitForWidget calls
    # can be replaced with the lower level qt_findWidgets. waitForWidget can be
    # used, but it's currently stored in a helper .tcl file which would need to
    # be sourced in the the cmd step below
    set cmd {
        # Find the Open File Dialog
        set dlg [qt_findWidgets -type dialog -text {Open File}];
        # Find the Open Button
        set btn [qt_findWidgets -parent $dlg -type button -text Open];
        # The text entries don't have unique objectNames so we'll just grab the first one because we know it's the file name field
        set entry [qt_findWidgets -parent $dlg -type qlineedit -nth 0]
        # Set the filename we want to load
        qt_setText $entry "./tests/TestGui/Automated/test_file.txt"
        qt_clickButton $btn;
    }

    # queue the commands we want to run after the modal file dialog is opened
    # these will run after 1000ms
    qt_queue_cmds 1000 $cmd
    # Bring up the Open File Dialog via the File > Open File... menu entry
    qt_openMenu File {Open File...}

    # Now that the dialog has been operated on, we can resume normal testing.
    # Note, we use waitForWidget instead of qt_findWidgets now that we are
    # back to tcl execution where we want might need to wait for the UI
    set editorTab [waitForWidget -type editor -text "test_file.txt"]
} on error {msg} {
    # Currently --script blocks the output of these messages, but foedag.log
    # will still capture them. Would be nice if we could somehow get the errors
    # to show up on failure somehow.
    puts "$msg"
    exit 1
}

# If we make it this far the test passed
exit 0



