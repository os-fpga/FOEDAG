import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import Qt.labs.platform 1.1

ApplicationWindow
{
    width: 350
    height: 250
    visible: windowModel.visible
    title: qsTr("AURORA")

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
      nameFilters: windowModel.newFileDialogFilters()
      fileMode: FileDialog.SaveFile
      selectedNameFilter.index: 0

      onAccepted:{
          windowModel.createNewFile(newFileDialog.file,
                                    newFileDialog.selectedNameFilter.extensions[newFileDialog.selectedNameFilter.index])
      }
   }

   FileDialog
   {
      id: openFileDialog
      title: qsTr("Open Project")
      nameFilters: windowModel.openFileDialogFilters()
      fileMode: FileDialog.OpenFile
      selectedNameFilter.index: 0

      onAccepted:{
          windowModel.createNewFile(newFileDialog.file,
                                    newFileDialog.selectedNameFilter.extensions[newFileDialog.selectedNameFilter.index])
      }
   }

   NewProjectDialog
   {
       id: newProjectDialog
   }
}
