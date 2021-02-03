#include<iostream>
#include<sstream>
#include<fstream>
#include<string>
#include"include/handler.h"

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

using namespace std;
using namespace cv;

//QElapsedTimer gTimer;


//
// Process Larva video, removing BG, detecting moving larva- Setting the learning rate will change the time required
// to remove a pupa from the scene -
//
unsigned int processVideo(mainwindow& window_main, trackerState& trackerState)
{
    //cv::namedWindow("Trackerdisplay",cv::WINDOW_NORMAL | cv::WINDOW_KEEPRATIO);
    //cv::resizeWindow("Trackerdisplay",250,250);


    QElapsedTimer otLastUpdate; //Time Since Last Progress Report
    otLastUpdate.start();

    cv::Mat frame,outframe;
    unsigned int nFrame         = 0;
    unsigned int nErrorFrames   = 0;

    QString frameNumberString;
    //OpenCV open The video File
    QFileInfo videoFile = trackerState.getNextVideoFile();
    if (!videoFile .exists())
    {
        window_main.LogEvent("[ERROR] Failed to find video file",1);
        return 0;
    }else
        window_main.LogEvent(QString("Processing video file") + videoFile.fileName() ,1);

    cv::VideoCapture cvcapture(videoFile.filePath().toStdString());
    if(!cvcapture.isOpened())
    {
        //error in opening the video input
        window_main.LogEvent("[ERROR] Failed to open video capture device",1);
        //std::cerr << gTimer.elapsed()/60000.0 << " [Error] Unable to open video file: " << videoFilename.toStdString() << std::endl;
        return 0;
        //std::exit(EXIT_FAILURE);
    }

    trackerState.setVidFps( cvcapture.get(CAP_PROP_FPS) );
    trackerState.setTotalFrames(cvcapture.get(CV_CAP_PROP_FRAME_COUNT));
    trackerState.setStopFrame(cvcapture.get(CV_CAP_PROP_FRAME_COUNT));
    window_main.LogEvent(QString("Total frames to track:") + QString::number( trackerState.getTotalFrames() ),5 );

    //  Check If it contains no Frames And Exit
    if (trackerState.getTotalFrames() < 2)
    {
        window_main.LogEvent("[ERROR] This Video File is empty ",2);
        cvcapture.release();
        return 0;
    }

    //read input data. ESC or 'q' for quitting
    while( !trackerState.bExiting && (char)trackerState.userInputKey != 27 )
    {
        /// Flow Control Code  - For When Looking at Specific Frame Region ///
        // 1st Check If user changed Frame - and go to that frame
        if (trackerState.bStartFrameChanged)
        {
            nFrame = trackerState.getCurrentFrameNumber();
            cvcapture.set(CV_CAP_PROP_POS_FRAMES,nFrame);
        }

        if (!trackerState.bPaused  )
        {
            nFrame = cvcapture.get(CV_CAP_PROP_POS_FRAMES);
           //window_main.tickProgress();
        }

        //if (trackerState.bPaused || trackerState.bStartFrameChanged)
        //    continue;
     //        trackerState.bStartFrameChanged = false; //Reset

        ///READ FRAME - Check For Error
         try //Try To Read The Image of that video Frame
        {
            //read the current frame
            if(!cvcapture.read(frame))
            {
                if (trackerState.atFirstFrame())
                {
                    window_main.LogEvent("# [Error]  Unable to read first frame.",2);
                    nFrame = 0; //Signals To caller that video could not be loaded.
                    trackerState.setCurrentFrameNumber(0);
                    //Delete the Track File //
                    //removeDataFile(outdatafile);
                    exit(EXIT_FAILURE);
                }
                else //Not Stuck On 1st Frame / Maybe Vid Is Over?>
                {
                   window_main.LogEvent("# [Error]  Unable to read first frame.",2);
                   if (!trackerState.atLastFrame() )
                   {
                       window_main.LogEvent("[Error] Stopped Tracking before End of Video - Delete Data File To Signal its Not tracked",3);
                       //removeDataFile(outdatafile); //Delete The Output File
                   }
                   else
                   {
                       window_main.LogEvent(" [info] processVideo loop done!",5);
                       //::saveImage(frameNumberString,QString::fromStdString( gTrackerState.gstroutDirCSV),videoFilename,outframe);
                   }
                   //continue;
                   break;
               }
            } //Can't Read Next Frame

        }catch(const std::exception &e)
        {

            window_main.LogEvent(QString(e.what()) + QString(" [Error]  reading frame. Skipping."),2);
            if (!trackerState.atLastFrame())
                cvcapture.set(CV_CAP_PROP_POS_FRAMES,nFrame+1);

            trackerState.setCurrentFrameNumber(nFrame+1);
            trackerState.errorFrames++;

            if (trackerState.errorFrames > 20) //Avoid Getting Stuck Here
            {
                // Too Many Error / Fail On Tracking
                 window_main.LogEvent(" [Error]  Problem with Tracking Too Many Read Frame Errors - Stopping Here and Deleting Data File To Signal Failure",1);
                //removeDataFile(outdatafile);
                break;
            }
            else
                continue;
        }
        //Update Frame Index
        nFrame = cvcapture.get(CV_CAP_PROP_POS_FRAMES);
        trackerState.setCurrentFrameNumber(nFrame);

        //Check If StopFrame Reached And Pause
        if (trackerState.atStopFrame() && !trackerState.bPaused)
        {
             trackerState.bPaused = true; //Stop Here
             window_main.LogEvent(QString(">>Stop Frame Reached - Video Paused<<"),5);
        }

         //Pause on 1st Frame If Flag Start Paused is set
         if (trackerState.bStartPaused && trackerState.atFirstFrame() && !trackerState.bPaused)
         {
             trackerState.bPaused = true; //Start Paused //Now Controlled By bstartPaused
             window_main.LogEvent(QString("[info]>> Video Paused<<"),5);
         }

         /// Start Processing The Frame
         outframe = frame.clone();
        //Process And Track  Tail

        //cv::imshow("Trackerdisplay",frame );

        // Display video Image on GUI
        window_main.showCVImage(outframe, nFrame);

         trackerState.processInputKey(cv::waitKey(1));



         QCoreApplication::processEvents(QEventLoop::AllEvents);
     }/// Main While - Reading Frames Loop


    return (trackerState.getCurrentFrameNumber());
}

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

    mainwindow omeanWindow(engine);



    // Make Another window Instance
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



    //int i=1505;
    //int imax=atoi(argv[2]);
//    string folder=argv[1];
//    string prefix="";
//    if(argc==4) prefix=argv[3];



    processVideo(omeanWindow,oTrackerstate);




//    char c=' ';
//    mouse_GetVector_param p; p.status=false;
//    cv::setMouseCallback("display1",mouse_GetVector,&p);
//    Mat draw;
//    Mat kernel = (Mat_<float>(3,3) << 1,  1, 1,
//                                      1, -8, 1,
//                                      1,  1, 1);

//    while(c!='q'){
//            stringstream ss;
//            ss<<folder<<"/"<<prefix<<"0.tiff";
//            Mat img=imread(ss.str().c_str(),IMREAD_UNCHANGED);
//            img.copyTo(draw);
//            if(p.status) {
//                arrowedLine(draw,p.pt1,p.pt2,2);
//                circle(draw,p.pt2,12,2);
//            }
//            imshow("display1",draw);
//            c=waitKey(10);
//    }
//    destroyWindow("display1");


//    namedWindow("display2",cv::WINDOW_NORMAL | cv::WINDOW_KEEPRATIO);
//    resizeWindow("display2",800,600);
//    cvCreateTrackbar( "frame", "display2", &i, imax,  NULL);
//    c='1';
//    while(c!='q'){
//        stringstream ss;
//        ss<<folder<<'/'<<prefix<<i<<".tiff";


//        Mat img=imread(ss.str().c_str(),IMREAD_UNCHANGED);
//        Mat ones(img.rows,img.cols,CV_32F,Scalar(1));
//        img.convertTo(img,CV_32F,1./255);
//        vector<Point2i> a_pts;
//        Point2d tangent;
//        tangent=p.pt2-p.pt1;

//        Mat draw, imgLaplacian,mask, draw_inv;
//        GaussianBlur(img,draw,Size(5,5),5,5);
//        filter2D(draw,imgLaplacian,CV_32F,kernel);
//        threshold(imgLaplacian, mask,0,1,THRESH_BINARY);
//        mask.convertTo(mask,CV_8U);
//        draw_inv=ones-draw;
//        draw_inv.copyTo(draw_inv,mask);
//        get_interp3(draw_inv,p.pt1,tangent,10,a_pts);

//        for(unsigned int j=0;j<a_pts.size()-1;++j) line(draw,a_pts[j],a_pts[j+1],255,1);
//        imshow("display2",draw);
//        c=waitKey(10);
//    }

//    return 0;

   return app.exec();
}


