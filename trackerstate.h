#ifndef TRACKERSTATE_H
#define TRACKERSTATE_H

#include <QGuiApplication>
#include <QApplication>
#include <QWidget>
#include <QDebug>
#include <QString>
#include <QFile>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QFileDialog>
#include <QElapsedTimer>


#include <string.h>
#include <fstream>
#include <iostream>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/video/background_segm.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/core/utility.hpp>
#include <opencv2/videoio.hpp>

typedef std::pair<QString,int> t_tracker_error;


/// \brief defines points along our custom linear spline that is fitted along the fish contour
typedef struct
{
    float x;
    float y; ///Position of Joint In Global Coordinates
    float angleRad;/// In Rads
    float spineSegLength;/// In pixels Float
} splineKnotf;

typedef std::vector<splineKnotf> t_fishspline;



/// \brief Process user provided config params and set corresponding internal/global variables
class trackerState
{
public:
    trackerState();
    ~trackerState();
    trackerState(cv::CommandLineParser& parser); //Read Command Line/Config Options
    void initBGSubstraction();
    int initInputVideoStream();
    void initSpine();
    void saveState(std::string strFilename);
    void loadState(std::string strFilename);
    void setVidFps(float fps);
    uint getCurrentFrameNumber();
    void setCurrentFrameNumber(uint nFrame);
    uint getStopFrame();
    void setStopFrame(uint nFrame);
    float getVidFps();
    void setTotalFrames(uint nFrames);
    uint getTotalFrames();
    QFileInfo getNextVideoFile();
    cv::Mat getNextFrame(); //Advance And Return A Video Frame
    bool atFirstFrame();
    bool atLastFrame();
    bool atStopFrame();
    void processInputKey(char Key);
    t_tracker_error getLastError();

public:
    bool bPaused     = true;
    bool bStartPaused = true;
    bool bROIChanged = true;
    bool bshowMask   = false;
    bool bTracking   = true; //Start By Tracking by default
    bool bExiting    = false;
    bool bStartFrameChanged = false;

    // Video Processing Flags
    bool bUseBGMOGModelling = true;
    bool bRemovePixelNoise = false;
    int g_Segthresh = 0;
    int userInputKey = 0;
    uint startFrame;
    uint endFrame;
    uint errorFrames = 0;

    //Background model
    cv::Ptr<cv::BackgroundSubtractorMOG2> pBGsubmodel; //MOG2 Background subtractor
    int MOGhistory = 200;
    float MOGBGRatio = 5.0f;
    int MOGNMixtures = 200;
    double MOGLearningRate = 0.00001;

    const int FitTailIntensityScanAngleDeg   = 60; //
    const int FishTailSpineSegmentCount      = 12;
    int FishTailSpineSegmentLength           = 10; //Length of Each Segment
    double fishBearingRads                 = 0; //Larval Orientation
    cv::Point ptTailRoot                   = cv::Point(80,30);
    int FitTailConfigState                 = 0; //A state Machine  register

    t_fishspline tailsplinefit; ///X-Y Coordinates of Fitted spline to contour

private:
    QDir outdir;
    QDir videodir;
    QFile outdatafile;
    QFileInfo invideofile;
    QStringList invidFileList; // List of video file names To process
    std::ofstream foutLog;//Used for Logging To File
    t_tracker_error lastError;
    cv::VideoCapture* pcvcapture;
   //
   float vidfps;
   uint totalVideoFrames;
   uint currentFrameNumber; //The current frame number of tracking


    //Morphological Kernels
    cv::Mat currentFrame;
    cv::Mat kernelOpen;
    cv::Mat kernelDilateMOGMask;
    cv::Mat kernelOpenfish;
    cv::Mat kernelClose;


};

#endif // TRACKERSTATE_H
