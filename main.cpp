#define _DEBUG

#include<iostream>
#include<sstream>
#include<fstream>
#include<string>


#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QDirIterator>
#include <QDir>
#include <QTextObject>
#include <QTextItem>
#include <QtQuick>
#include <QDebug>
#include <QQmlComponent>
//#include <QThread>
#include <QTime>
#include <QElapsedTimer>

////Open CV
//#include <opencv2/opencv_modules.hpp> //THe Cuda Defines are in here
//#include <opencv2/opencv.hpp>
//#include <opencv2/core/core.hpp>
//#include "opencv2/core/utility.hpp"
//#include <opencv2/highgui/highgui.hpp>
//#include <opencv2/imgproc/imgproc.hpp>
//#include <opencv2/features2d.hpp>
//#include <opencv2/video/background_segm.hpp>

#include <trackerstate.h>
#include <trackerimageprovider.h>
#include <mainwindow.h>
#include <handler.h>

using namespace std;
using namespace cv;

//QElapsedTimer gTimer;


int main(int argc, char* argv[]){


    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication app(argc, argv);

    QQmlApplicationEngine engine;
    const QUrl mainwindow_url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [mainwindow_url](QObject *obj, const QUrl &objUrl) {
        if (!obj && mainwindow_url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.load(mainwindow_url);

    trackerImageProvider* ptrackerView = new trackerImageProvider();

    engine.addImageProvider(QLatin1String("trackerframe"),ptrackerView);

     // To make a new window Instance:
    //QQmlComponent mainWindow(&engine,url);
   // QObject *mainW = mainWindow.create();



  /// Handle Command Line Parameters //
    const cv::String keys =
        "{help h usage ? |    | print this help  message}"
        "{outputdir   o |    | Dir where To save sequence of images }"
        "{invideofile v |    | Behavioural Video file to analyse }"
        "{invideolist f |    | A text file listing full path to video files to process}"
        "{logtofile l |    | Filename to save clog stream to }"
        "{ModelMOG b | 1  | Learn and Substract Stationary Objects from Foreground mask}"
        "{SkipTracked t | 0  | Skip Previously Tracked Videos}"
        "{FilterPixelNoise pn | 0  | Filter Pixel Noise During Tracking (Note:This has major perf impact so use only when necessary due to pixel noise. BGProcessing does it by default)}"
        "{DisableOpenCL ocl | 0  | Disabling the use of OPENCL can avoid some SEG faults hit when running multiple trackers in parallel}"
        "{TrackFish ft | 1  | Track Fish not just the moving prey }"
        "{StartPaused | 1  | Do not initiate tracking upon start.}"
        "{BGThreshold bgthres | 30  | Absolute grey value used to segment background}"
        ;

    ///Parse Command line Args
    cv::CommandLineParser parser(argc, argv, keys);

    stringstream ssMsg;
    ssMsg<<"Larval Zebrafish Tail Tracker"<< std::endl;
    ssMsg<<"--------------------------" << std::endl;
    ssMsg<<"Author : Konstantinos Lagogiannis 2021, King's College London"<<std::endl;
    ssMsg<< "email: costaslag@gmail.com"<<std::endl;
    ssMsg<<"./2pTailTrack <outfolder> <inVideoFile> "<<std::endl;
    ssMsg<<"(note: output folder is automatically generated when absent)"<<std::endl;
    ssMsg << "Example: \n : " << std::endl;
    ssMsg << "./2pTailTrack  -o=./Tracked30-11-21/" << std::endl;
    ssMsg << "-Make Sure QT can be found : use export LD_LIBRARY_PATH= path to Qt/5.11.1/gcc_64/lib/  " << std::endl;
    parser.about(ssMsg.str() );


   trackerState oTrackerstate(parser);
   mainwindow omeanWindow(engine,&oTrackerstate);

    //int i=1505;
    //int imax=atoi(argv[2]);
//    string folder=argv[1];
//    string prefix="";
//    if(argc==4) prefix=argv[3];

//   ///Open Output File Check If We Skip Processed Files
//   if ( !openDataFile(outputFileName,invideoname,outfishdatafile) )
//   {
//        if (gTrackerState.bSkipExisting) //Failed Due to Skip Flag
//             continue; //Do Next File
//   }else
//       writeFishDataCSVHeader(outfishdatafile);

    processVideo(omeanWindow,oTrackerstate);

   return app.exec();
}



void writeFishDataCSVHeader(QFile& data,trackerState& trackerState)
{

    /// Write Header //
    QTextStream output(&data);
    output << "frameN \t tailLengthpx \t ThetaSpine_0 \t ";
    for (int i=1;i<trackerState.FishTailSpineSegmentCount;i++)
        output <<  "DThetaSpine_" << i << "\t";

    output << "\n";

}


bool openDataFile(QString filepathCSV,QString filenameVid,QFile& data,QString strpostfix,trackerState& trackerState,mainwindow& omeanWindow)
{
    int Vcnt = 1;
    bool newFile = false;
    //Make ROI dependent File Name
    QFileInfo fiVid(filenameVid);
    QFileInfo fiOut(filepathCSV+"/") ;
    QString fileVidCoreName = fiVid.completeBaseName();
    QString dirOutPath = fiOut.absolutePath() + "/"; //filenameCSV.left(filenameCSV.lastIndexOf("/")); //Get Output Directory

    //strpostfix = strpostfix + "_%d.csv";


    //char buff[50];
    //sprintf(buff,strpostfix.toStdString(),Vcnt);
    //dirOutPath.append(fileVidCoreName); //Append Vid Filename To Directory
    //dirOutPath.append(buff); //Append extension track and ROI number
    if (fileVidCoreName.contains(strpostfix,Qt::CaseSensitive))
    {
        fileVidCoreName = fileVidCoreName.left(fileVidCoreName.lastIndexOf("_"));
        dirOutPath = dirOutPath + fileVidCoreName+ "_" + QString::number(Vcnt) +  ".csv";
    }
    else
        dirOutPath = dirOutPath + fileVidCoreName + strpostfix + "_" + QString::number(Vcnt) + ".csv";

    data.setFileName(dirOutPath);
    //Make Sure We do not Overwrite existing Data Files
    while (!newFile)
    {
        if (!data.exists() || data.isOpen()) //Write HEader
        {
            newFile = true;
        }else{
            //File Exists
            if (trackerState.bSkipExisting)
            {
                omeanWindow.LogEvent("[warning] Output File Exists and SkipExisting Mode is on.",3);
                std::cerr << "Skipping Previously Tracked Video File" << std::endl;
                return false; //File Exists Skip this Video
            }
            else
            {
                //- Create Name
            //Filename Is Like AutoSet_12-10-17_WTNotFedRoti_154_002_tracks_1.csv
                //Increase Seq Number And Reconstruct Name
                Vcnt++;
                // If postfix (track / food) already there, then just add new number
                if (fileVidCoreName.contains(strpostfix,Qt::CaseSensitive))
                    dirOutPath = fiOut.absolutePath() + "/" + fileVidCoreName + "_" + QString::number(Vcnt) + ".csv";
                else
                    dirOutPath = fiOut.absolutePath() + "/" + fileVidCoreName + strpostfix + "_" + QString::number(Vcnt) + ".csv";



                data.setFileName(dirOutPath);
                //data.open(QFile::WriteOnly)

            }
         }
    }
    if (!data.open(QFile::WriteOnly |QFile::Append))
    {
        std::cerr << "Could not open output file : " << data.fileName().toStdString() << std::endl;
        return false;
    }else {
        //New File
        //if (!trackerState.bBlindSourceTracking)
        std::clog << "Opened file " << dirOutPath.toStdString() << " for data logging." << std::endl;

        //output.flush();

    }

    return true;
}

