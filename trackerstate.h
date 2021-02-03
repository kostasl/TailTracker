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

/// \brief Process user provided config params and set corresponding internal/global variables
class trackerState
{
public:
    trackerState();
    trackerState(cv::CommandLineParser& parser); //Read Command Line/Config Options
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
    bool atFirstFrame();
    bool atLastFrame();
    bool atStopFrame();
    void processInputKey(int Key);

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

private:
    QDir outdir;
    QDir videodir;
    QFile outdatafile;
    QFileInfo invideofile;
    QStringList invidFileList; // List of video file names To process
    std::ofstream foutLog;//Used for Logging To File

   //
   float vidfps;
   uint totalVideoFrames;
   uint currentFrame; //The current frame number of tracking


    //Morphological Kernels
    cv::Mat kernelOpen;
    cv::Mat kernelDilateMOGMask;
    cv::Mat kernelOpenfish;
    cv::Mat kernelClose;

};

#endif // TRACKERSTATE_H
