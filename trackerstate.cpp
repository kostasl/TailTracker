#include "trackerstate.h"

using namespace std;

trackerState::trackerState()
{
    initSpine();

}
trackerState::~trackerState()
{
    delete pcvcapture;
    kernelOpen.release();
    kernelDilateMOGMask.release();
    kernelOpenfish.release();
    kernelClose.release();

}

void trackerState::setVidFps(float fps)
{
    vidfps = fps;
}

float trackerState::getVidFps()
{
    return(vidfps);
}


void trackerState::setTotalFrames(uint nFrames)
{
    totalVideoFrames = nFrames;
}

uint trackerState::getTotalFrames()
{
    return(totalVideoFrames);
}


uint trackerState::getCurrentFrameNumber()
{
    //if (!bStartFrameChanged)
    currentFrameNumber = pcvcapture->get(CV_CAP_PROP_POS_FRAMES);

    return(currentFrameNumber);
}

void trackerState::setCurrentFrameNumber(uint nFrame)
{

    bStartFrameChanged = (nFrame !=currentFrameNumber);
    bPaused = true;

    pcvcapture->set(CV_CAP_PROP_POS_FRAMES,nFrame);
    currentFrameNumber = nFrame;
}

/// \brief Return the next image from the source video
cv::Mat  trackerState::getNextFrame()
{
    cv::Mat nextFrame;

    //Return Last Captured Frame
    if (bPaused && !atFirstFrame() )
        return (currentFrame); //Stick to the current Frame

    ///READ FRAME - Check For Error
     try //Try To Read The Image of that video Frame
    {
        //read the current frame
        if(!pcvcapture->read(nextFrame))
        {
            if (atFirstFrame())
            {
                //window_main.LogEvent("# [Error]  Unable to read first frame.",2);
                lastError.first = "[ERROR]  Unable to read first frame.";
                lastError.second = 2;
                currentFrameNumber = 0; //Signals To caller that video could not be loaded.

                setCurrentFrameNumber(currentFrameNumber+1); //Skip
                //Delete the Track File //
                //removeDataFile(outdatafile);
                //exit(EXIT_FAILURE);
            }
            else //Not Stuck On 1st Frame / Maybe Vid Is Over?>
            {
                lastError.first = "[Error]  Unable to read first frame.";
                lastError.second = 2;
               if (!atLastFrame())
               {
                   //window_main.LogEvent("[Error] Stopped Tracking before End of Video - Delete Data File To Signal its Not tracked",3);
                   //removeDataFile(outdatafile); //Delete The Output File
                   lastError.first = "[Error] Stopped Tracking before End of Video - Delete Data File To Signal its Not tracked";
                   lastError.second = 3;
               }
               else { ///Reached last Frame Video Done
                   //window_main.LogEvent(" [info] processVideo loop done!",5);
                   lastError.first = "[INFO] processVideo loop done!";
                   lastError.second = 5;
                   //::saveImage(frameNumberString,QString::fromStdString( gTrackerState.gstroutDirCSV),videoFilename,outframe);
               }
               //continue;
           }
        }else /// ERROR Reading Frame - Can't Read Next Frame
            currentFrameNumber++;
    }catch(const std::exception &e)
    {

        //window_main.LogEvent(QString(e.what()) + QString(" [Error]  reading frame. Skipping."),2);
        lastError.first = QString(e.what()) + QString(" [Error]  reading frame. Skipping.");
        lastError.second = 2;

        if (!atLastFrame())
            setCurrentFrameNumber(currentFrameNumber++);
            //pcvcapture->set(CV_CAP_PROP_POS_FRAMES,currentFrame+1);
        errorFrames++;

        if (errorFrames > 20) //Avoid Getting Stuck Here
        {
            // Too Many Error / Fail On Tracking
            lastError.first = "[ERROR]  Problem with Tracking Too Many Read Frame Errors - Stopping Here and Deleting Data File To Signal Failure";
            lastError.second = 5;
             //window_main.LogEvent("[ERROR]  Problem with Tracking Too Many Read Frame Errors - Stopping Here and Deleting Data File To Signal Failure",1);
            //removeDataFile(outdatafile);
        }
    }

    /// Done Retrieving next Frame /Save if not empty Good
    if (!nextFrame.empty())
        nextFrame.copyTo(currentFrame);

    return(nextFrame);
}
void trackerState::setStopFrame(uint nFrame)
{
    endFrame = nFrame;
}

uint trackerState::getStopFrame()
{
    return(endFrame);
}


bool trackerState::atFirstFrame()
{
    return(currentFrameNumber == startFrame);
}
bool trackerState::atLastFrame()
{
    return(currentFrameNumber == (totalVideoFrames-1));
}
bool trackerState::atStopFrame()
{
    return(currentFrameNumber == endFrame);
}

void trackerState::processInputKey(char Key)
{
    userInputKey = (int)Key;

    switch (Key)
    {
     case 27:
     case 'Q':
     case 'q':
            bExiting = true;
            bTracking = false;
            bPaused = true;
            break;

    case 'T':
    case 't':
        bTracking = true;
        break;

    case 'P':
    case 'p':
        bPaused = true;
        std::clog << "[INFO] Paused " << std::endl;
        break;

    case 'R':
    case 'r':
        bPaused = false;

    }
}

//Return Last Recorded error and Remove error
t_tracker_error trackerState::getLastError()
{
  t_tracker_error tmp_lastError = lastError;
  lastError.first = "OK";
  lastError.second = 10; //Lowest Priority
  return(tmp_lastError);
}

/// \brief Initialize BG substractor objects, depending on options / can use cuda
int trackerState::initInputVideoStream()
{
    QFileInfo videoFile = getNextVideoFile();
    int ret = 0;
    if (!videoFile .exists())
    {
        lastError.first = "[ERROR] Failed to find video file";
        lastError.second = 1;
        return 0;
    }else
    {
        lastError.first = QString("Processing video file") + videoFile.fileName();
        lastError.second = 9;
    }

    if (videoFile.suffix() == "avi" || videoFile.suffix() == "mp4" || videoFile.suffix() == "mkv")
    {
        std::clog << "Attempting to open video file " << videoFile.fileName().toStdString() << std::endl;

        pcvcapture = new cv::VideoCapture(videoFile.filePath().toStdString());
        if(!pcvcapture->isOpened())
        {
            //error in opening the video input
            lastError.first = "[ERROR] Failed to open video capture device";
            lastError.second = 1;
            ret = 0;
        }else{
            lastError.first = "[INFO] video capture device opened succesfully";
            lastError.second = 10;
            ret = 1;
        }

        setVidFps(pcvcapture->get(CV_CAP_PROP_FPS) );
        setTotalFrames(pcvcapture->get(CV_CAP_PROP_FRAME_COUNT));
        setStopFrame(pcvcapture->get(CV_CAP_PROP_FRAME_COUNT));

      }//If File is a video

    //  Check If it contains no Frames And Exit
    if (getTotalFrames() < 2)
    {
        lastError.first = "[ERROR] This Video File is empty ";
        lastError.second = 2;
        ret = 0;
        pcvcapture->release();
    }

    // Video  Paused upon opening
    if (bStartPaused)
        bPaused = true;

    return ret;
}//End of Function

/// \brief Initialize BG substractor objects, depending on options / can use cuda
void trackerState::initBGSubstraction()
{

    //Doesn't matter if cuda FLAG is enabled
    pBGsubmodel =  cv::createBackgroundSubtractorMOG2(MOGhistory, 3,true);

    pBGsubmodel->setHistory(MOGhistory);
    pBGsubmodel->setNMixtures(MOGNMixtures);
    pBGsubmodel->setBackgroundRatio(MOGBGRatio);

    cv::Mat frame,fgMask;


    //Get BGImage
    while (getCurrentFrameNumber() < MOGhistory)
    {
        frame = getNextFrame();

        //LearningRate Negative parameter value makes the algorithm to use some automatically chosen learning rate
        pBGsubmodel->apply(frame,fgMask,MOGLearningRate);

        pBGsubmodel->getBackgroundImage(bgFrame);
        cv::imshow("fg MAsk Learning",fgMask);
        cv::imshow("BG Model Learning",bgFrame);

        QCoreApplication::processEvents(QEventLoop::AllEvents);
    }

    setCurrentFrameNumber(1); //Reset To First Frame
    //Make Learning Nominal
    MOGLearningRate = 0.0001;



}



/// \return FileInfo of the next video file on the list, if it exists / Otherwise empty fileinfo structure.
QFileInfo trackerState::getNextVideoFile()
{
    if (invidFileList.count() >0)
    {
        invideofile.setFile(invidFileList.first());
        if (!invideofile.exists())
        {
            std::cerr << "Error video file missing " << invideofile.path().toStdString() << std::endl;
        }
        invidFileList.removeFirst();

        return (invideofile);
    }

    return(QFileInfo());
}

/// \brief Process user provided config params and set corresponding internal/global variables
trackerState::trackerState(cv::CommandLineParser& parser):trackerState()
{

    QString stroutFilename;
    QString strvidFilename;
    QString stroutDir;

    if (parser.has("help"))
    {
        parser.printMessage();
        return;
    }

    //Check if outdir provided otherwise open Dialog
    if (parser.has("outputdir"))
    {
        std::string soutFolder   = parser.get<std::string>("outputdir");
    }
    else
    {
      // getting the filename (full path)

      stroutFilename  = QFileDialog::getSaveFileName(nullptr, "Save tracks to output","VX_pos.csv", "CSV files (*.csv);", nullptr, nullptr);
      stroutDir = stroutFilename.left(stroutFilename.lastIndexOf("/"));
    }

    if (!outdir.exists((stroutDir)))
        outdir.mkdir((stroutDir));
    outdir.setPath((stroutDir));

    std::clog << "Output Data file : " << stroutFilename.toStdString() << std::endl;
    //std::cout << "Output Data file : " << stroutFilename.toStdString()  << "\n " << std::endl;


 /// Check if vid file provided in arguments.
 /// If File exists added to video file list, otherwise save directory and open dialogue to choose a file from there
    if (parser.has("invideofile"))
    {   QString fvidFileName = QString::fromStdString( parser.get<std::string>("invideofile") );
        invideofile.setFile(fvidFileName) ;
        videodir.setPath(fvidFileName);

        if (invideofile.exists() && invideofile.isFile())
            invidFileList.append( invideofile.filePath() );
    }else {
        if (invidFileList.empty())
                invidFileList = QFileDialog::getOpenFileNames(nullptr, "Select videos to Process", outdir.path() ,
                                                              "Video file (*.mpg *.avi *.mp4 *.h264 *.mkv *.tiff *.png *.jpg *.pgm)", nullptr, nullptr);
            //Retain ref to output directory
            videodir.setPath(invidFileList.first());
            invideofile = invidFileList.first();
    }


    if (parser.has("invideolist"))
    {
        qDebug() << "Load Video File List " <<  QString::fromStdString(parser.get<std::string>("invideolist"));
        QFile fvidfile( QString::fromStdString(parser.get<std::string>("invideolist")) );
        if (fvidfile.exists())
        {
            fvidfile.open(QFile::ReadOnly);
            //QTextStream textStream(&fvidfile);
            while (!fvidfile.atEnd())
            {
                QString line = fvidfile.readLine().trimmed();
                if (line.isNull())
                    break;
                else
                    invidFileList.append(line);
            }
        }else
        {
            qWarning() << fvidfile.fileName() << " does not exist!";
        }
    }

    /// Setup Output Log File //
    if ( parser.has("logtofile") )
    {
        qDebug() << "Set Log File To " <<  QString::fromStdString( parser.get<std::string>("logtofile") );

        QFileInfo oLogPath( QString::fromStdString(parser.get<std::string>("logtofile") ) );
        if (!oLogPath.absoluteDir().exists())
            QDir().mkpath(oLogPath.absoluteDir().absolutePath()); //Make Path To Logs

        foutLog.open(oLogPath.absoluteFilePath().toStdString());
         // Set the rdbuf of clog.
         std::clog.rdbuf(foutLog.rdbuf());
         std::cerr.rdbuf(foutLog.rdbuf());
    }

    // Read In Flag To enable Fish Tracking / FishBlob Processing
    //Check If We Are BG Modelling / BEst to switch off when Labelling Hunting Events
    if (parser.has("ModelMOG"))
         bUseBGMOGModelling = (parser.get<int>("ModelMOG") == 1)?true:false;

    //if (parser.has("SkipTracked"))
    //     bSkipExisting = (parser.get<int>("SkipTracked") == 1)?true:false;

       if (parser.has("BGThreshold"))
           g_Segthresh = parser.get<int>("BGThreshold");

    if (parser.has("FilterPixelNoise"))
    {
        bRemovePixelNoise = (parser.get<int>("FilterPixelNoise") == 1)?true:false;
        std::clog << "Remove Pixel Noise Filter Is " << ((bRemovePixelNoise )?"On":"Off") << std::endl;
    }

    if (parser.has("StartPaused"))
         bStartPaused = (parser.get<int>("StartPaused") == 1)?true:false;

     //uiStartFrame = parser.get<uint>("startframe");
     //uiStopFrame = parser.get<uint>("stopframe");

    ///* Create Morphological Kernel Elements used in processFrame *///
    kernelOpen          = cv::getStructuringElement(cv::MORPH_CROSS,cv::Size(1,1),cv::Point(-1,-1));
    kernelDilateMOGMask = cv::getStructuringElement(cv::MORPH_ELLIPSE,cv::Size(3,3),cv::Point(-1,-1));
    kernelOpenfish      = cv::getStructuringElement(cv::MORPH_ELLIPSE,cv::Size(3,3),cv::Point(-1,-1)); //Note When Using Grad Morp / and Low res images this needs to be 3,3
    kernelClose         = cv::getStructuringElement(cv::MORPH_ELLIPSE,cv::Size(5,5),cv::Point(-1,-1));

    /// create Gaussian Smoothing kernels for Contour //
    //assert(dGaussContourKernelSize % 2 == 1); //M is an odd number
    //getGaussianDerivs(dGaussContourKernelSigma,dGaussContourKernelSize,gGaussian,dgGaussian,d2gGaussian);

}
/// END OF INIT GLOBAL PARAMS //

void trackerState::initSpine()
{
    //Reset Legth
    int c_spineSegL =  FishTailSpineSegmentLength;

    tailsplinefit.clear();
    tailsplinefit.reserve(FishTailSpineSegmentCount);

    for (int i=0;i<FishTailSpineSegmentCount;i++)
    {

        splineKnotf sp;
        //1st Spine Is in Opposite Direction of Movement and We align 0 degrees to be upwards (vertical axis)
        //if (this->bearingRads > CV_PI)
        if (fishBearingRads < 0)
            fishBearingRads  += 2.0*CV_PI;

            sp.angleRad    = (fishBearingRads)-CV_PI; //   //Spine Looks In Opposite Direction
            sp.spineSegLength = c_spineSegL;    //Default Size
            if (sp.angleRad < 0)
                sp.angleRad += 2.0*CV_PI;
        //else
//            sp.angleRad    = (this->bearingRads)+CV_PI/2.0; //CV_PI/2 //Spine Looks In Opposite Direcyion

        assert(!std::isnan(sp.angleRad && std::abs(sp.angleRad) <= 2*CV_PI && (sp.angleRad) >= 0 ));

        if (i==0)
        {
            sp.x =  ptTailRoot.x;
            sp.y =  ptTailRoot.y;
        }
        else
        {
            //0 Degrees Is vertical Axis Looking Up
            sp.x        = tailsplinefit[i-1].x + ((double)c_spineSegL)*sin(sp.angleRad);
            sp.y        = tailsplinefit[i-1].y - ((double)c_spineSegL)*cos(sp.angleRad);
        }

        tailsplinefit.push_back(sp); //Add Knot to spline
    }



//    //    ///DEBUG
//        for (int j=0; j<c_spinePoints;j++) //Rectangle Eye
//        {
//            cv::circle(frameDebugC,cv::Point(spline[j].x,spline[j].y),2,TRACKER_COLOURMAP[j],1);
//        }

       // drawSpine(frameDebugC);
       // cv::waitKey(300);

}
