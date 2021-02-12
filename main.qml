import QtQuick 2.12
import QtQuick.Window 2.10
import QtQuick.Controls 2.3
//import QtQuick.Dialogs.qml 1.0
import QtQuick.Dialogs 1.2

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
        source: "qrc:/MeyerLogoIcon256x256.png"
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


    FileDialog {
        id: fileDialogInput
        objectName: "inputVideoFile"
        title: "Please choose a video file"
        signal qmlInputFileSelectedSig()
        folder: shortcuts.home
        onAccepted: {
            console.log("You chose: " + inputVideoFile.fileUrls)
            qmlInputFileSelectedSig();
            Qt.quit()
        }
        onRejected: {
            console.log("Canceled")
            Qt.quit()
        }
        Component.onCompleted: visible = false
    }
    FileDialog {
        id: fileDialogOutput
        objectName: "inputOutFile"
        title: "Please choose a save file"
        folder: shortcuts.home
        signal qmlOutputFileSelectedSig()
        onAccepted: {
            console.log("You chose: " + fileDialogOutput.fileUrls)
            qmlOutputFileSelectedSig();
            Qt.quit()
        }
        onRejected: {
            console.log("Canceled")
            Qt.quit()
        }
        Component.onCompleted: visible = false
    }

    Button {
        id: buttonInput
        x: 401
        y: 26
        onPressed: {
            fileDialogInput.visible = true;
        }

        text: qsTr("Select Input Video")
    }

    Button {
        id: buttonOutput
        x: 401
        y: 91
        width: 140
        height: 40
        onPressed: {
            fileDialogOutput.visible = true;
        }
        text: qsTr("Select output file")
    }

    Button {
        id: buttonTrack
        x: 401
        y: 159
        width: 140
        height: 40
        signal qmlStartTracking();
        text: qsTr("Start tracking")
        onPressed: {
             console.log("Starting Tracking...");
            qmlStartTracking();
        }
    }

}
