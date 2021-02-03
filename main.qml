import QtQuick 2.12
import QtQuick.Window 2.12

Window {
    objectName:"mainWindow"
    visible: true
    width: 640
    height: 480
    color: "#729fcf"
    title: qsTr("2p microscope camera tail tracker ")
    signal qmlSignal(string msg)

    Text {
        id: txtLog
        objectName: "txtLog"
        x: 7
        y: 387
        width: 626
        height: 75
        text: qsTr("Tracker activity Log ")
        transformOrigin: Item.TopLeft
        styleColor: "#588076"
        opacity: 0.807
        font.pixelSize: 12
        signal qmlSignal(string msg)
    }

    Connections {
        target: txtLog
        onTextChanged: txtLog.qmlSignal("onTextChanged")
    }

    Image {
        id: videoImage
        objectName: "imgTracker"
        x: 33
        y: 34
        width: 250
        height: 250
        fillMode: Image.PreserveAspectFit
        source: "qrc:/qtquickplugin/images/template_image.png"
    }

    MouseArea {
        id: mouseArea
        x: 18
        y: 22
        width: 295
        height: 292
    }
}
