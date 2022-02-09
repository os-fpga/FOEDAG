import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15

ApplicationWindow
{
    width: 350
    height: 250
    visible: true
    title: qsTr("FOEDAG")

    menuBar: MainMenuBar
    {
        id: _mainMenu
    }

    header: MainToolBar
    {
    }

    footer: ToolBar
    {
        Text {
            id: status
            text: qsTr("Status")
        }
    }

   Rectangle
   {
       anchors.fill: parent
       color: "green"
   }
}
