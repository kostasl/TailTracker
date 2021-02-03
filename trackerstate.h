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


class trackerState
{
public:
    trackerState();
    trackerState(cv::CommandLineParser& parser); //Read Command Line/Config Options
    void saveState(std::string strFilename);
    void loadState(std::string strFilename);
    void setVidFps(float fps);
    float getVidFps();
    void setTotalFrames(uint nFrames);
    uint getTotalFrames();
    QFileInfo getNextVideoFile();
    /// \brief Process user provided config params and set corresponding internal/global variables


private:
    QDir outdir;
    QDir videodir;
    QFile outdatafile;
    QFileInfo invideofile;
    QStringList invidFileList; // List of video file names To process
    std::ofstream foutLog;//Used for Logging To File

    bool bPaused     = true;
    bool bStartPaused = true;
    bool bROIChanged = true;
    bool bshowMask   = false;
    bool bTracking   = true; //Start By Tracking by default
    bool bExiting    = false;

    // Video Processing Flags
    bool bUseBGMOGModelling = true;
    bool bRemovePixelNoise = false;
    int g_Segthresh = 0;

   //
   float vidfps;
   uint totalVideoFrames;
   uint currentFrame;

    //Morphological Kernels
    cv::Mat kernelOpen;
    cv::Mat kernelDilateMOGMask;
    cv::Mat kernelOpenfish;
    cv::Mat kernelClose;

};

#endif // TRACKERSTATE_H
