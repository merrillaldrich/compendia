import QtQuick

Item {
    id: item1
    x: -40
    y: -31
    width: 120
    height: 150

    Image {
        id: image
        x: 0
        y: 0
        width: 120
        height: 120
        source: "../SampleFiles/Hilde at the Dragon.jpg"
        fillMode: Image.PreserveAspectFit
    }

    Text {
        id: filename
        x: 26
        y: 126
        text: qsTr("Filename.jpg")
        font.pixelSize: 12
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignTop
        transformOrigin: Item.Top
    }
}
