import QtQuick

Rectangle {
    id: tag_pill
    width: 100
    height: 25
    color: "#efb19b"
    radius: 12
    transformOrigin: Item.TopLeft

    Text {
        id: tag_text
        x: 24
        y: (parent.height / 2) - (height / 2)
        text: qsTr("Text")
        font.pixelSize: 14
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        transformOrigin: Item.Left
    }
}
