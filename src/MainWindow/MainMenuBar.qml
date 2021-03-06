import QtQuick 2.15
import QtQuick.Controls 2.15

MenuBar
{
    Menu {
        title: qsTr("&File")
        MenuItem
        {
            id: menuItemNew
            text: qsTr("&New")
            icon.source: "qrc:/images/icon_newfile.png"
            icon.color: "transparent"
            onTriggered:
            {
                newFileDialog.open()
            }

            Shortcut{
                sequence: StandardKey.New
                enabled: true
                onActivated: menuItemNew.triggered()
            }
        }
        MenuItem
        {
            id: menuItemNewProject
            text: qsTr("&NewProject")
            onTriggered:
            {
                newProjectDialog.show()
            }
        }
        MenuItem
        {
            id: menuItemOpen
            text: qsTr("&OpenProject")
            onTriggered:
            {
                openFileDialog.open()
            }
            Shortcut{
                sequence: StandardKey.Open
                enabled: true
                onActivated: menuItemOpen.triggered()
            }
        }
        MenuSeparator {}
        MenuItem
        {
            text: qsTr("&Exit")
            Shortcut{
                sequence: StandardKey.Quit
                onActivated: Qt.quit()
            }
        }
    }
    Menu {
        title: qsTr("&Help")
        MenuItem { text: qsTr("&About") }
    }
}
