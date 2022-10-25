# Load our helper commands
source tests/Harness/ui_testing_cmds.tcl

# If run via --replay, add gui_start to load the UI
#gui_start

# For generic widgets that don't have an objectName, qt_findWidgets allows you to query for the widget
# Here we are getting the New file button on the toolbar which doesn't have an objectName
# Note that qt_findWidgets automatically removes & from qpushbuttons and qactions which might have a format like &New
set newBtn [qt_findWidgets -type button -text "New"]
# at this point, newBtn will contain some pointer string like "QWidget(0x12345678)"
# now we can pass that "pointer" to qt_clickButton. (Not going to call this currently so following steps can be run)
# qt_clickButton $newBtn

# For generic, duplicate widgets, the -nth argument can be used
# This will open the settings dialog by clicking the first instance of "Edit Settings..." in the task window
set settings [waitForWidget -type button -text "Edit settings..." -nth 0]
qt_clickButton $settings

# As more widgets in our UI have their objectName set, queries for the widget can be made more accurate
# Note: Dialog's are top level widgets that require their own special search, as such, -type dialog is always required when looking for a qdialog
# Note: Currently, only non-modal dialogs work, the following line only passes if the associated dlg has been changed to non-mmodal (from ->exec() to ->show())
set dlg [waitForWidget -type dialog -objectName tasksDlg_Synthesis__SettingsDialog]

# Generic widgets like an OK button can use the -parent argument to limit the search scope
set ok [waitForWidget -parent $dlg -type button -text ok]
qt_clickButton $ok

# Not everything waits for widgets currently and you might run into a scenario where you need so delay for the test to pass
# adding sleep X gives the ui a bit more time to finish its last operation before executing the next steps
sleep 1000

# Menus can be traverse by providing the text of each menu entry and the subsequent child entry etc
# Note: in tcl, to pass a whitespace separated string as 1 argument, wrap it in {}
qt_openMenu File {Recent Projects} noname

