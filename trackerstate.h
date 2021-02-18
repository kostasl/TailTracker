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



#include <trackerimageprovider.h>

//class trackerImageProvider; //fwd declaration
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
    trackerState(cv::CommandLineParser& parser,trackerImageProvider* ptrackerView); //Read Command Line/Config Options
    void initBGSubstraction();
    int initInputVideoStream();
    bool unloadCurrentVideo();
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
    trackerImageProvider* ImageSequenceProvider() const;
    void processInputKey(char Key);
    t_tracker_error getLastError();
    bool openDataFile();
    void closeDataFile();
    bool saveFrameTrack();
    void saveOutputframe(cv::Mat& outframe); //To save Tracked Frame back out to file

    /// \brief called after user selects input folder or file - Function appands to list of videos for processing -
    /// when a directory with an image sequence is added, the  ImageProvider detects and adds the image files in the directory
    void addinputFile(QString filepath){

        invideofile.setFile(filepath);
        videodir.setPath(filepath);
        //This is a single video file
        if (invideofile.exists() && invideofile.isFile())
            invidFileList.append( invideofile.filePath() );
        //Adding Folder Not File
        if (videodir.exists() && invideofile.isDir())
        {
            QFileInfoList inputfileList = videodir.entryInfoList(InputFilefilters,QDir::Filter::Files);
            if (!inputfileList.isEmpty()){
                invidFileList.append(inputfileList.first().absoluteFilePath());
            }
            else{
                lastError.first = "Folder " + videodir.path() + " is empty.";
            }
        }


        //Retain ref to output directory
        if (!invidFileList.isEmpty())
        {
            videodir.setPath(invidFileList.first());
            //invidFileList.append(filepath);
            //invideofile.

        }


        qDebug() << filepath << " Added to queue";
    }
    void setOutputFile(QString filepath)
    {
        closeDataFile(); //Close If Already Open

        outdir.setPath((filepath));
        if (!outdir.exists((filepath)))
            outdir.mkdir((filepath));
        if (invideofile.exists())
            outdatafile.setFileName(outdir.path() +"/"+ invideofile.fileName().append(".csv"));
        else
            outdatafile.setFileName(outdir.path() +"/output.csv");

        qDebug() << "Output file set to:" << filepath;

        openDataFile(); //Open New one
    }
    bool startTracking();
    bool isReady();





public:
    bool bPaused     = true;
    bool bStartPaused = true;
    bool bROIChanged = true;
    bool bshowMask   = false;
    bool bTracking   = true; //Start By Tracking by default
    bool bBGReady    = false;
    bool bExiting    = false;
    bool bReady      = false;
    bool bStartFrameChanged     = false;
    bool bSkipExisting          = false; //If An output file exists then do not retrack the video / Skip
    bool brecordTrackerFrames    = false; /// Save output images files from GUI frame showing spine
    // Video Processing Flags
    bool bUseBGMOGModelling = true;
    bool bRemovePixelNoise = false;
    int g_Segthresh = 0;
    int userInputKey = 0;
    uint startFrame = 1;
    uint endFrame; /// User defined tracking stop point /  Before Video Ends
    uint errorFrames = 0;

    //Background model
    cv::Ptr<cv::BackgroundSubtractorMOG2> pBGsubmodel; //MOG2 Background subtractor
    const int c_MOGhistory = 2000;
    int MOGhistory = 2000;
    ///The main threshold on the squared Mahalanobis distance to decide if the sample is well described by the background model or not.
    /// //If a pixel is not close to any component, it is considered foreground or added as a new component. 3 sigma => Tg=3*3=9 is default.
    double MOGVarThreshold = 3.0;
    //If a foreground pixel keeps semi-constant value (?) for about backgroundRatio*history frames,
    //it's considered background and added to the model as a center of a new component. It corresponds to TB parameter in the paper.
    float MOGBGRatio = 0.01f;
    int MOGNMixtures = 12;
    double MOGLearningRate = 0.01; //0.0001
    const double c_MOGLearningRate = 0.01; //0.0001
    const double MOGNominamLearningRate = 0.0;
    cv::Mat bgFrame;

    //double contrastGain = 3.4; //In image Provider
    //double brightness  = 25;

    const int FitTailIntensityScanAngleDeg      = 60; //35
    int FishTailSpineSegmentCount               = 15;
    int FishTailSpineSegmentLength              = 14; //Length of Each Segment
    double fishBearingRads                      = 26.0*CV_PI/180.0; //Larval Orientation (Assume 0 Is Vertical Y axis) -Default
    cv::Point ptTailRoot                        = cv::Point(115,53);
    int FitTailConfigState                      = 0; //A state Machine  register

    t_fishspline tailsplinefit; ///X-Y Coordinates of Fitted spline to contour


    QDir outdir; ///Where CSV and Image Frames are saved

protected:
    trackerImageProvider* mptrackerView; //pointer to external ImageSequence Provider
    void writeFishDataCSVHeader(QFile& data);

private:
    QDir videodir;
    QFile outdatafile;
    QStringList InputFilefilters;



    QFileInfo invideofile;
    QFileInfo lastvideofile;
    QStringList invidFileList; // List of video file names To process
    std::ofstream foutLog;//Used for Logging To File
    t_tracker_error lastError;
    cv::VideoCapture* pcvcapture;
   //
   //float vidfps;
   //uint totalVideoFrames;
   //uint currentFrameNumber; //The current frame number of tracking


    //Morphological Kernels
    cv::Mat currentFrame;
    cv::Mat kernelOpen;
    cv::Mat kernelDilateMOGMask;
    cv::Mat kernelOpenfish;
    cv::Mat kernelClose;


};

#endif // TRACKERSTATE_H
