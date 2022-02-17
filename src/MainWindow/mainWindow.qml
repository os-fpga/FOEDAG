import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Dialogs 1.1

ApplicationWindow
{
    width: 350
    height: 250
    visible: windowModel.visible
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
            text: windowModel.statusBarMessage
        }
    }

   Rectangle
   {
       anchors.fill: parent
       color: "green"

      Text
      {
          text: "Visible is " + windowModel.visible
      }
   }

   //declaration
   FileDialog
  {
      id: newFileDialog
      title: qsTr("New File")
      nameFilters: windowModel.fileDialogFilters()
      selectExisting: false

      onAccepted:
      {
          windowModel.createNewFile(newFileDialog.fileUrl)
          console.log("New file " + newFileDialog.fileUrl)
      }
      onRejected:
      {

      }
  }
}
