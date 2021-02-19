#ifndef TRACKERIMAGEPROVIDER_H
#define TRACKERIMAGEPROVIDER_H
#include <iostream>

#include <QQuickImageProvider>
#include <QImage>
#include <QPixmap>
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>

//Open CV
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/video/background_segm.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/core/utility.hpp>
#include <opencv2/videoio.hpp>

//#include <trackerstate.h>

typedef std::pair<QString,int> t_tracker_error;

enum class sourceVideoTypes
{
    ImageSequence,
    VideoFile,
    ImageDirectory
};

class  trackerImageProvider : public QQuickImageProvider
{

    friend class trackerState; //So it can access members for frame

public:
    trackerImageProvider(): QQuickImageProvider(QQuickImageProvider::Pixmap)
    {
         //currentFrame = cv::Mat::zeros(200,200,CV_8U );
        //pvideoDirIt =

    }
    ~trackerImageProvider(){
        closeInputStream();
    }
    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize) override;

    //Hold local copy of next opencv Tracker Frame
    bool atFirstFrame();
    bool atLastFrame();
    bool atStopFrame();
    float getVidFps();
    void setNextFrame(QPixmap frm);
    void setNextFrame(cv::Mat frm);
    cv::Mat getNextFrame();
    cv::Mat getCurrentFrame();
    uint getTotalFrames();
    uint getCurrentFrameNumber();
    uint endFrameNumber(); //Use Image Sequence file names to obtain last Frame Number
    void setCurrentFrameNumber(uint nFrame);
    int initInputVideoStream(QFileInfo& videofile);
    int initInputVideoStream(QString filename);
    void closeInputStream();
    void setBrightness(double B){
        brightness = B;
        qDebug() << "Brighness adjust set to :" << B;
    }
    void setConstrast(double C){
        qDebug() << "Contrast adjust set to :" << C;
        contrastGain  = C;
    }

    t_tracker_error getLastError();


    t_tracker_error lastError; //Holds data on the last error from the last function call of this object
    uint startFrame = 0;
    uint endFrame = 0;

private:
    cv::Mat currentFrame;
   QPixmap pixmap;
   QFileInfo videoFile;
   QDir videoDir;
   //Smart Pointer
   std::unique_ptr<QDirIterator>  pvideoDirIt{new QDirIterator("")};
   //QFileInfoList imageSequenceFiles;
    QStringList imageSequenceFilenames;
    QStringList imageSequenceFilenames_orig;
   cv::VideoCapture* pcvcapture;


protected:
    uint errorFrames = 0;
    float vidfps;
    uint totalVideoFrames;

    uint currentFrameNumber = 0; //The current frame number of tracking

    double contrastGain = 2.4;
    double brightness  = 45;

    bool bStartFrameChanged;
    sourceVideoTypes inputSourceMode;
};

#endif // TRACKERIMAGEPROVIDER_H
