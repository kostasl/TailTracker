import QtQuick 2.12
import QtQuick.Window 2.10
import QtQuick.Controls 2.3

Window {
    objectName:"mainWindow"
    visible: true
    width: 640
    height: 480
    color: "#729fcf"
    title: qsTr("Tail tracker for 2p-microscope constrained larva videos")
    signal qmlSignal(string msg)
    onSceneGraphInitialized: busyIndicator.running = false;
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
                  if(status == Image.Ready)
                      busyIndicator.running = false;
                      //console.log("BG Calculation Done. Ready to track.")
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

                  BusyIndicator {
                      id: busyIndicator
                      objectName: "BusyIndicator"
                      anchors.centerIn: parent
                      running: true
                      x: 70
                      y: 70
                  }

        }


    }
}
