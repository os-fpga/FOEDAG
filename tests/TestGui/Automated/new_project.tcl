# Load our helper commands
source tests/Harness/ui_testing_cmds.tcl

# This is an example of how we might work w/ the New Project wizard.
# It will open the wizard via the menu, add the aes_decrypt directory,
# and then finish the dialog.

# The test will auto close on success, but you can comment out the last exit
# line to see that the project has been created and the file tree shows the
# choosen files

# Having to launch a modal dialog from within a modal dialog means we have to
# nest multiple queue commands. This is obviously a pretty messy test and we
# should look into how we might abstract all the nesting and the error
# handling try/catches

# Currently the gui keeps the tcl script from erroring out so we have to try catch the entire test, should explore a better solution for this
try {
    # Queue commands that need to be run while a modal dialog is blocking
    set cmd {
        try {
            # Find the New Project Dialog
            set dlg [waitForWidget -type dialog -text {New Project}];
            # Find the Next Button
            set next [waitForWidget -parent $dlg -type button -text Next];
            qt_clickButton $next;
            # need to wait a bit for the UI to update, could also try waitForObject, but that would require sourcing the harness commands in this cmd block (for now at least)
            sleep 1000
            qt_clickButton $next;
            set addDirsBtn [waitForWidget -parent $dlg -type button -text "Add Directory..."]

            # Create a set of commands load aes_decrypt files, after "Add Directory..." is clicked
            set cmd2 {
                try {
                    # Find the Dialog that was just raised
                    set dirDlg [waitForWidget -type dialog -text {Select Directory}];
                    # QFileDialog text entries don't have unique objectNames so we'll just grab the first one because we know it's the folder name field
                    set entry [waitForWidget -parent $dirDlg -type qlineedit -nth 0]
                    # set the path to the folder we want
                    qt_setText $entry "./tests/Testcases/aes_decrypt_fpga"
                    # confirm the dialog
                    set choose [waitForWidget -parent $dirDlg -type button -text "Choose"]
                    qt_clickButton $choose
                } on error {msg} {
                    puts "error while executing queued commands: $msg"
                    exit 1
                }
            }
            # Queue the Add Directory commands we want to fire once the dialog is blocking
            qt_queue_cmds 1000 $cmd2
            # Click "Add Directory..."
            qt_clickButton $addDirsBtn;

            # Navigate through the rest of the pages
            qt_clickButton $next
            sleep 1000
            qt_clickButton $next
            sleep 1000
            qt_clickButton $next
            sleep 1000
            # On the last page, click the finish button
            set finish [waitForWidget -parent $dlg -type button -text "Finish"]
            qt_clickButton $finish

            set done 1
        } on error {msg} {
            puts "error while executing queued commands: $msg"
            exit 1
        }
    }

    # queue the commands we want to run after the modal file dialog is opened
    # these will run after 1000ms
    qt_queue_cmds 1000 $cmd
    # Bring up the New Project Dialog via the File > New Project... menu entry
    qt_openMenu File {New Project...}

    # needs more investigation, but currently it appears we need to await our queue'd commands at the top level otherwise the test will finish before the commands run
    # currently achieving this by vwait'ing a variable name done and then setting that at the end of our queued commands
    # we should try to find a way to encapsulate this so the dev doesn't have to bother with it
    vwait done
    puts "Test ran without error"
} on error {msg} {
    # Currently --script blocks the output of these messages, but foedag.log
    # will still capture them. Would be nice if we could somehow get the errors
    # to show up on failure somehow.
    puts "$msg"
    exit 1
}

# this test should verify that the appropriate files were added to the project etc, but we don't have commands for that yet

# pass if we make it this far
exit 0



