import QtQuick 2.0
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15

ToolBar {
    RowLayout {
        anchors.fill: parent
        ToolButton {
            icon.source: "qrc:/images/icon_newfile.png"
            icon.color: "transparent"
            onClicked: newFileDialog.open()
        }
    }
}
