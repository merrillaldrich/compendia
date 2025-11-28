

/*
This is a UI file (.ui.qml) that is intended to be edited in Qt Design Studio only.
It is supposed to be strictly declarative and only uses a subset of QML. If you edit
this file manually, you might introduce QML code that is not supported by Qt Design Studio.
Check out https://doc.qt.io/qtcreator/creator-quick-ui-forms.html for details on .ui.qml files.
*/
import QtQuick
import QtQuick.Controls
import Luminism

Rectangle {
    width: Constants.width
    height: Constants.height

    color: Constants.backgroundColor

    ListView {
        id: listView
        x: 566
        y: 340
        width: 160
        height: 80
        model: ListModel {
            ListElement {
                name: "Red"
                colorCode: "red"
            }

            ListElement {
                name: "Green"
                colorCode: "green"
            }

            ListElement {
                name: "Blue"
                colorCode: "blue"
            }

            ListElement {
                name: "White"
                colorCode: "white"
            }
        }
        delegate: Row {
            spacing: 5
            Rectangle {
                width: 100
                height: 20
                color: colorCode
            }

            Text {
                width: 100
                text: name
            }
        }
    }

    GroupBox {
        id: groupBox
        x: 627
        y: 513
        width: 200
        height: 200
        title: qsTr("Group Box")
    }

    Button {
        id: button
        x: 667
        y: 743
        text: qsTr("Button")
    }

    Row {
        id: row
        x: 925
        y: 349
        width: 329
        height: 23

        Text {
            id: text1
            text: qsTr("Text")
            font.pixelSize: 12
            rightPadding: 5
            leftPadding: 5
        }

        Text {
            id: text2
            text: qsTr("Text")
            font.pixelSize: 12
            rightPadding: 5
            leftPadding: 5
        }

        Text {
            id: text3
            text: qsTr("Text")
            font.pixelSize: 12
            rightPadding: 5
            leftPadding: 5
        }
    }
}
