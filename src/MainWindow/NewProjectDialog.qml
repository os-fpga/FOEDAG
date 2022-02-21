import QtQuick 2.0
import Qt.labs.platform 1.1
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Window
{
    property int currentIndex: 0
    readonly property int startIndex: 0
    readonly property int lastIndex: 5
    readonly property int marginValue: 20

    width: Screen.width/3
    height: Screen.height/3
    title: qsTr("New Project")
    modality: Qt.WindowModal

    function nextId()
    {
        if (currentIndex == 0) {currentIndex++; return pageProjectType}
        if (currentIndex == 1) {currentIndex++; return pageAddSource}
        if (currentIndex == 2) {currentIndex++; return pageAddConstraint}
        if (currentIndex == 3) {currentIndex++; return pageAddDevicePlanner}
        if (currentIndex == 4) {currentIndex++; return pageSummary}
        if (0 > currentIndex >= 5) {console.log("Out of range")}
    }

    onVisibilityChanged:
    {
        newProjectStack.push(pageLocation)
    }
    Rectangle
    {
        id: stackViewRect
        anchors.top: parent.top
        height: parent.height * 0.9
        width: parent.width

       StackView
       {
           id: newProjectStack
           initialItem: pageLocation
           anchors.fill: parent
           anchors.margins: marginValue

           onCurrentItemChanged:
           {
               if (currentIndex == startIndex)
                   backButton.enabled = false
               else
                   backButton.enabled = true

               if (currentIndex == lastIndex)
                   nextOrFinishButton.text = qsTr("Finish")
               else
                   nextOrFinishButton.text = qsTr("Next")
           }
       }
    }
    Rectangle
    {
        id: buttonsRect
        anchors.top: stackViewRect.bottom
        height: parent.height * 0.1
        width: parent.width

        Row
        {
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            anchors.rightMargin: 15
            spacing: 15
            Button
            {
                id: backButton
                text: "Back"
                onClicked:
                {
                    currentIndex--
                    newProjectStack.pop()
                }
            }
            Button
            {
                id: nextOrFinishButton
                onClicked:
                {
                    if (currentIndex == lastIndex)
                        {}
                    else
                        newProjectStack.push(nextId())
                }
            }
            Button
            {
                text: "Cancel"
                onClicked: hide()
            }
        }
    }
    //Stack view components (pages)
    //page 0. Location
    Component
    {
        id: pageLocation
        Rectangle
        {
            Rectangle
            {
                id:top1
                height: parent.height - bottom1.height

                Column
                {
                    Text
                    {
                        id: caption0
                        font.bold: true
                        font.pointSize: 12
                        text: windowModel.pageHeadCaption(currentIndex)
                    }
                    Text
                    {
                        width: stackViewRect.width - marginValue*2
                        text: windowModel.locationPageMainText()
                        wrapMode: Text.Wrap
                    }
                }
            }
            Rectangle
            {
                id:bottom1
                anchors.top: top1.bottom
                anchors.margins: 20
                height: 200

                function updatePathToProject()
                {
                    pathToProject.text = windowModel.projectFullPathCaption() + windowModel.fullPathToProject()
                }
                function urlToPath(urlString) {
                    var s
                    if (urlString.startsWith("file:///")) {
                        var k = urlString.charAt(9) === ':' ? 8 : 7
                        s = urlString.substring(k)
                    } else {
                        s = urlString
                    }
                    return decodeURIComponent(s);
                }
                GridLayout
                {
                    id: grid
                    width: stackViewRect.width - marginValue*2
                    columns: 3
                    rows: 2
                    Text{text: windowModel.projectNameCaption()}
                    TextField
                    {
                        id: projectNameTextField
                        text: windowModel.projectName
                        placeholderText: "Enter project name here"
                        Layout.columnSpan: 2
                        Layout.fillWidth: true
                        onTextChanged:
                        {
                            windowModel.projectName = text
                            bottom1.updatePathToProject()
                        }
                    }
                    Text{text: windowModel.projectLocationCaption()}
                    TextField
                    {
                        id: projectDirPathTextField
                        text: windowModel.projectLocation
                        placeholderText: "Browse or enter project location here"
                        Layout.fillWidth: true
                        onTextChanged:
                        {
                            windowModel.projectLocation = text
                            bottom1.updatePathToProject()
                        }
                    }
                    Button
                    {
                        id: browseButton
                        text: "Browse..."
                        onClicked:
                        {

                            dialog1.open()
                        }
                    }
                    FolderDialog
                    {
                       id: dialog1
                       title: qsTr("Select directory")

                       onAccepted:{
                            projectDirPathTextField.text = bottom1.urlToPath(dialog1.folder.toString())
                       }
                    }
                }
                CheckBox
                {
                    id: checkBoxCreateSubdir
                    anchors.top: grid.bottom
                    text: windowModel.checkBoxSubDirectoryCaption()
                    onCheckStateChanged:
                    {
                        windowModel.needToCreateProjrctSubDirectory = checkState
                        bottom1.updatePathToProject()
                    }
                }
                Text
                {
                    id: pathToProject
                    anchors.top: checkBoxCreateSubdir.bottom
                    text: windowModel.projectFullPathCaption() + windowModel.fullPathToProject()
                }
            }
        }
    }
    //page 1. Progect Type
    Component
    {
        id: pageProjectType
        Text {
            id: caption0
            font.bold: true
            font.pointSize: 12
            text: windowModel.pageHeadCaption(currentIndex)
        }
    }
    //page 2. Add Source
    Component
    {
        id: pageAddSource
        Text {
            id: caption0
            font.bold: true
            font.pointSize: 12
            text: windowModel.pageHeadCaption(currentIndex)
        }
    }
    //page 3. Add Constraint
    Component
    {
        id: pageAddConstraint
        Text {
            id: caption0
            font.bold: true
            font.pointSize: 12
            text: windowModel.pageHeadCaption(currentIndex)
        }
    }
    //page 4. Add Device Planner
    Component
    {
        id: pageAddDevicePlanner
        Text {
            id: caption0
            font.bold: true
            font.pointSize: 12
            text: windowModel.pageHeadCaption(currentIndex)
        }
    }
    //page 5. Summary
    Component
    {
        id: pageSummary
        Text {
            id: caption0
            font.bold: true
            font.pointSize: 12
            text: windowModel.pageHeadCaption(currentIndex)
        }
    }
}
