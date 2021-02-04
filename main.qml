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
//    onContentItemChanged: {
//            qmlSignal("Change");
//    }


    Text {
        id: txtLog
        objectName: "txtLog"
        x: 7
        y: 330
        width: 626
        height: 132
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
        x: 195
        y: 38
        width: 200
        height: 200
        fillMode: Image.PreserveAspectFit
        source: "image://trackerframe/0"
        onStatusChanged: {
               //   if(status == Image.Ready)
                      //indicator.running = false;
                      //console.log("Img")
              }

        MouseArea {
                  width: 200
                  height: 200
                  objectName: "imgMouseArea"
                  anchors.fill: parent
                  acceptedButtons: Qt.LeftButton | Qt.RightButton
                  anchors.rightMargin: 0
                  anchors.bottomMargin: 0
                  anchors.leftMargin: 0
                  anchors.topMargin: 0
                  hoverEnabled : true
                  signal qmlMouseClickSig()
                  signal qmlMouseDragSig()
                  signal qmlMouseReleased()
                  signal qmlMouseMoved()
                  onClicked: {
                      //indicator.running = true;
                      //console.log("irregular area clicked");
                      //qmlMouseClickSig();
                      //videoImage.source = "image://trackerframe/" + Math.random(1000)

                  }
                  //onDoubleClicked: qmlMouseClickSig()
                  onPressAndHold:{
                      cursorShape = Qt.CrossCursor;
                      qmlMouseClickSig();
                      hoverEnabled = true;
                  }
                  onPositionChanged: {
                      //Draging Motion
                      qmlMouseDragSig();
                  }
                  onReleased:{
                      //Draging Motion
                      qmlMouseReleased();
                      //hoverEnabled = false;
                      cursorShape = Qt.ArrowCursor;
                  }

        }

//        BusyIndicator {
//            id: indicator
//            anchors.centerIn: parent
//            running: false
//        }


    }
}
