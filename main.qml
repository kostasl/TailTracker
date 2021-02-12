import QtQuick 2.12
import QtQuick.Window 2.10
import QtQuick.Controls 2.3
//import QtQuick.Dialogs.qml 1.0
import QtQuick.Dialogs 1.2

Window {
    id:mainWindow
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

//    Connections {
//        target: txtLog
//        onTextChanged: txtLog.qmlSignal("onTextChanged")
//    }

    Image {
        id: imgTracker
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
                  id: imgMouseArea
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
        objectName: "fileDialogInput"
        title: "Please choose a video file"
        signal qmlInputFileSelectedSig(string msg)
        selectedNameFilter: "*.pgm *.tiff * mp4 *.avi "
        selectExisting: true
        folder: shortcuts.home
        onAccepted: {
            console.log("You chose: " + fileDialogInput.fileUrls)
            qmlInputFileSelectedSig(fileDialogInput.fileUrls);
            Qt.quit()
        }
        onRejected: {
            console.log("Canceled")
            Qt.quit()
        }
        //onCompleted: visible = false
    }
    FileDialog {
        id: fileDialogOutput
        objectName: "fileDialogOutput"
        title: "Set output file "
        folder: shortcuts.home
        //selectFile : "somefile.txt"
        signal qmlOutputFileSelectedSig(string msg)
        selectExisting: false
        onAccepted: {
            console.log("You chose: " + fileDialogOutput.fileUrls)
            qmlOutputFileSelectedSig(fileDialogOutput.fileUrls);
            Qt.quit()
        }
        onRejected: {
            console.log("Cancelled")
            Qt.quit()
        }



         //Component.onCompleted: visible = false
    }


    Button {
        id: buttonOutput
        x: 465
        y: 220
        width: 149
        height: 40
        onPressed: {
            fileDialogOutput.selectFolder = true;
            fileDialogOutput.visible = true;
        }
        text: qsTr("Select output folder")
    }

    Button {
        id: buttonTrack
        objectName: "buttonStartTrack"
        x: 225
        y: 267
        width: 140
        height: 40
        signal qmlStartTracking();
        text: qsTr("Start tracking")
        onPressed: {
            console.log("Starting Tracking...");
            qmlStartTracking();
        }
    }

    Button {
        id: buttonOpenFolder
        x: 465
        y: 86
        width: 149
        height: 40
        text: qsTr("Select Input Folder")
        onPressed:
        {
            fileDialogInput.setTitle("Select Folder with image sequence");
            fileDialogInput.setNameFilters("*");
            fileDialogInput.selectNameFilter("*");
            fileDialogInput.setNameFilters("*");
            fileDialogInput.selectFolder = true;

            fileDialogInput.visible = true;
        }
    }

    Button {
        id: buttonSelectVideo
        x: 465
        y: 26
        width: 149
        height: 40
        onPressed: {
            fileDialogInput.setTitle("Select Video file");
            fileDialogInput.selectFolder = false;
            fileDialogInput.setNameFilters("*.avi *.mp4 *.mkv *.h264;; *.*");
            fileDialogInput.visible = true;
        }

        text: qsTr("Select Input Video")
    }

}
