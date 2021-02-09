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

    QGuiApplication::setQuitOnLastWindowClosed(true);

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
        "{startframe s | 1  | Video Will start by Skipping to this frame}"
        "{stopframe e | 0  | Video Will stop at this frame}"
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


    trackerImageProvider* ptrackerView = new trackerImageProvider();
    engine.addImageProvider(QLatin1String("trackerframe"),ptrackerView);
    trackerState oTrackerstate(parser,ptrackerView);
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

    if(oTrackerstate.bExiting)
    {
        omeanWindow.LogEvent("Goodbye!",0);
        cv::destroyAllWindows();

        app.quit();
        engine.quit();
        engine.exit(EXIT_SUCCESS);
        app.exit(EXIT_SUCCESS);


        //exit(0);
        //std::exit(EXIT_SUCCESS);
        return EXIT_SUCCESS;
    }

   return app.exec();
}



