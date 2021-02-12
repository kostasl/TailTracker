#include "trackerstate.h"
#include "handler.h"

using namespace std;

trackerState::trackerState()
{
    initSpine();

}
trackerState::~trackerState()
{
    //if (pcvcapture)
    //    delete pcvcapture;
    kernelOpen.release();
    kernelDilateMOGMask.release();
    kernelOpenfish.release();
    kernelClose.release();

}


/// \brief Process user provided config params and set corresponding internal/global variables
trackerState::trackerState(cv::CommandLineParser& parser, trackerImageProvider* ptrackerView ):trackerState()
{

    QString stroutFilename;
    QString strvidFilename;
    QString stroutDir;

    if (parser.has("help"))
    {
        parser.printMessage();
        return;
    }

 /// Check if vid file provided in arguments.
 /// If File exists added to video file list, otherwise save directory and open dialogue to choose a file from there
    if (parser.has("invideofile"))
    {   QString fvidFileName = QString::fromStdString( parser.get<std::string>("invideofile") );
        invideofile.setFile(fvidFileName) ;
        videodir.setPath(fvidFileName);

        if (invideofile.exists() && invideofile.isFile())
            invidFileList.append( invideofile.filePath() );
    }else {
        //QFileDialog::setFileMode(QFileDialog::Directory);

        if (invidFileList.empty())
               invidFileList = QFileDialog::getOpenFileNames(nullptr, "Select videos or images to process", outdir.path() ,
                                                              "Image sequences (*.tiff *.png *.jpg *.pgm);;Video files (*.mpg *.avi *.mp4 *.h264 *.mkv)", nullptr, nullptr);
            //Retain ref to output directory
        if (!invidFileList.isEmpty())
        {
            videodir.setPath(invidFileList.first());
            invideofile = invidFileList.first();
        }
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

    //Check if outdir provided otherwise open Dialog
    if (parser.has("outputdir"))
    {
        QString soutFolder = QString::fromStdString(parser.get<std::string>("outputdir"));
        if (!outdir.exists((soutFolder)))
            outdir.mkdir((soutFolder));
    }
    else
    {
      // getting the filename (full path)
      stroutFilename  = QFileDialog::getSaveFileName(nullptr, "Save tracks to output",invideofile.dir().path() +"/"+ invideofile.dir().dirName().append(".csv"), "CSV files (*.csv);", nullptr, nullptr);
      std::clog << "Output Data file : " << stroutFilename.toStdString() << std::endl;
      outdir.setPath((stroutFilename));
    }

    outdatafile.setFileName(stroutFilename);
    //std::cout << "Output Data file : " << stroutFilename.toStdString()  << "\n " << std::endl;

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

     startFrame = parser.get<uint>("startframe");
     endFrame = parser.get<uint>("stopframe");

    ///* Create Morphological Kernel Elements used in processFrame *///
    kernelOpen          = cv::getStructuringElement(cv::MORPH_CROSS,cv::Size(1,1),cv::Point(-1,-1));
    kernelDilateMOGMask = cv::getStructuringElement(cv::MORPH_ELLIPSE,cv::Size(3,3),cv::Point(-1,-1));
    kernelOpenfish      = cv::getStructuringElement(cv::MORPH_ELLIPSE,cv::Size(3,3),cv::Point(-1,-1)); //Note When Using Grad Morp / and Low res images this needs to be 3,3
    kernelClose         = cv::getStructuringElement(cv::MORPH_ELLIPSE,cv::Size(5,5),cv::Point(-1,-1));

    mptrackerView = ptrackerView;
    /// create Gaussian Smoothing kernels for Contour //
    //assert(dGaussContourKernelSize % 2 == 1); //M is an odd number
    //getGaussianDerivs(dGaussContourKernelSigma,dGaussContourKernelSize,gGaussian,dgGaussian,d2gGaussian);

}
/// END OF INIT GLOBAL PARAMS //


void trackerState::setCurrentFrameNumber(uint nFrame)
{
    bStartFrameChanged = (nFrame !=mptrackerView->getCurrentFrameNumber());
    bPaused = true;
    mptrackerView->setCurrentFrameNumber(nFrame);
}



/// Retrieves next frame from capture device -
/// \brief Transforms Brighness contrast and Return the next image from the source video
///
cv::Mat  trackerState::getNextFrame()
{

    cv::Mat nextFrame;

    //Return Last Captured Frame
    if (bPaused && !atFirstFrame() )
        return (currentFrame); //Stick to the current Frame

        nextFrame = mptrackerView->getNextFrame();
        lastError =  mptrackerView->getLastError();

     currentFrame = nextFrame;
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
    return(mptrackerView->getCurrentFrameNumber() == startFrame);
}
bool trackerState::atLastFrame()
{
    return(mptrackerView->getCurrentFrameNumber() == (mptrackerView->getTotalFrames()));
}
bool trackerState::atStopFrame()
{
    return(mptrackerView->getCurrentFrameNumber() == endFrame);
}

trackerImageProvider* trackerState::ImageSequenceProvider() const
{
    return (mptrackerView);
}



bool trackerState::saveFrameTrack(){

    if (bPaused)
        return (false);

    const double Rad2Deg = (180.0/CV_PI);

    QTextStream datStream(&outdatafile);

    //for (auto it = h.pointStack.begin(); it != h.pointStack.end(); ++it)
    datStream.setRealNumberNotation(QTextStream::RealNumberNotation::FixedNotation );
    datStream.setRealNumberPrecision(2);

    datStream << mptrackerView->getCurrentFrameNumber() <<"\t" << FishTailSpineSegmentLength*FishTailSpineSegmentCount << "\t";

    //Set Global 1st Spine Direction (Helps to detect Errors)
    assert(tailsplinefit.size() == FishTailSpineSegmentCount);

    datStream << Rad2Deg* tailsplinefit[0].angleRad;
    //Output Spine Point Angular Deviations from the previous spine/tail Segment in Degrees
    for (int i=1;i<FishTailSpineSegmentCount;i++)
    {
       datStream << "\t" << Rad2Deg*(tailsplinefit[i-1].angleRad - tailsplinefit[i].angleRad);
       // datStream << "\t" << Rad2Deg*angleBetween(cv::Point2f(tailsplinefit[i-1].x,tailsplinefit[i-1].y),  cv::Point2f(tailsplinefit[i].x,tailsplinefit[i].y));
    }
     datStream << "\n";

     return(true);
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
        if (!atLastFrame())
            bPaused = false;
        else
        {
           lastError.first = "[INFO] Cannot upause - End of video reached ";
           lastError.second = 10;
           std::clog << "[INFO] Cannot upause - End of video reached " << std::endl;
           mptrackerView->initInputVideoStream(invideofile);
           std::clog << "[INFO] Rewinding-video " << std::endl;
        }

    }
}

//Return Last Recorded error and Remove error
t_tracker_error trackerState::getLastError()
{
  t_tracker_error tmp_lastError = lastError;
  lastError.first = "OK";
  lastError.second = 0; //No Error
  return(tmp_lastError);
}

/// \brief Initialize BG substractor objects, depending on options / can use cuda
int trackerState::initInputVideoStream()
{
    QFileInfo videoFile = getNextVideoFile();

    if (videoFile.isDir())
       std::clog << "Reading image files from " << videoFile.path().toStdString() << std::endl;

    int ret    = mptrackerView->initInputVideoStream(videoFile);
    startFrame = mptrackerView->startFrame;
    lastError  = mptrackerView->getLastError();

    //move to requested start frame
    mptrackerView->setCurrentFrameNumber(startFrame);

    if (endFrame == 0) //If User has not set Stop Point- Set it to end of video
        endFrame =  mptrackerView->endFrameNumber();

    assert(startFrame < endFrame);

    //  Check If it contains no Frames And Exit
    if (mptrackerView->getTotalFrames() < 2)
    {
        lastError.first = "[ERROR] This Video File is empty ";
        lastError.second = 2;
        ret = 0;
        mptrackerView->closeInputStream();
    }

    // Video  Paused upon opening
    if (bStartPaused)
        bPaused = true;

    return ret;
}//End of Function



/// Do a prelim Run Through MOGHistory initial video frames and compute BG image
/// Save all BGImages on the way and then take the median image for the BG substraction
/// This BG image reduced in brightness and substracted from image before tracking the tail.
/// \brief Initialize BG substractor objects, depending on options / can use cuda
void trackerState::initBGSubstraction()
{

    //Cap Number of BG training Frames To not exceed number of available frames
    MOGhistory = (mptrackerView->getTotalFrames() < MOGhistory)?mptrackerView->getTotalFrames():MOGhistory;
    MOGLearningRate = max(MOGLearningRate,5.0/MOGhistory); //Adjust Learning Rate To Number of Available Frames

    std::vector<cv::Mat> listImages(MOGhistory);

    bPaused = false;
    //Doesn't matter if cuda FLAG is enabled
    pBGsubmodel =  cv::createBackgroundSubtractorMOG2(MOGhistory, MOGVarThreshold,false);

    pBGsubmodel->setHistory(MOGhistory);
    pBGsubmodel->setNMixtures(MOGNMixtures);
    pBGsubmodel->setBackgroundRatio(MOGBGRatio);

    cv::Mat frame,fgMask;//imgInit;

//    imgInit = cv::Mat::ones(200,200,CV_8UC1);
//    pBGsubmodel->apply(imgInit,fgMask,MOGLearningRate);

    int i = 0;
    //Get BGImage
    while (i < MOGhistory && (!atLastFrame()))
    {
        frame = mptrackerView->getNextFrame();
        lastError = mptrackerView->getLastError();
        if (lastError.second != 0) //Check for Error
        {
            std::clog << lastError.first.toStdString() << std::endl;
            continue;
        }
        //LearningRate Negative parameter value makes the algorithm to use some automatically chosen learning rate
        pBGsubmodel->apply(frame,fgMask,MOGLearningRate);
        pBGsubmodel->getBackgroundImage(listImages[i++]);
        //cv::imshow("fg MAsk Learning",fgMask);
        //Show Frame Being Processed
        mptrackerView->setNextFrame(listImages[i-1]);

        try{
             //an OpenGL context for  QSurfacerFormaIs  ERROR raised here on some systems
            QCoreApplication::processEvents(QEventLoop::AllEvents);
        }catch (...)
        {
             qDebug() << "processEvents Error - Switch QSurface Profile ";

             QSurfaceFormat format;
             format.setDepthBufferSize(24);
             format.setOption(QSurfaceFormat::DebugContext);
             QSurfaceFormat::setDefaultFormat(format);
             format.setProfile(QSurfaceFormat::CompatibilityProfile);
             qDebug() << "QSurfaceFormat Switched to Compatibility Profile";

             format.setRenderableType( QSurfaceFormat::OpenGL);
             QSurfaceFormat::setDefaultFormat( format );

        }
    }

    setCurrentFrameNumber(startFrame); //Reset To First Frame

    //Make Learning Nominal
    MOGLearningRate = MOGNominamLearningRate; //Back to Nominal Rate - So Almost no Learning Happens
    bPaused = bStartPaused;


    ///// GEt Median Image - SORT images By Pixel intensity //
    cv::Mat tmp;
    // We will sorting pixels where the first mat will get the lowest pixels and the last one, the highest
    for(int i = 0; i < listImages.size(); i++) {
        for(int j = i + 1; j < listImages.size(); j++) {
            listImages[i].copyTo(tmp);
            //Calculates per-element maximum/minimum of two arrays
            cv::min(listImages[i], listImages[j], listImages[i]);
            cv::max(listImages[j], tmp, listImages[j]);
//Uncomment the following to sort images by total intensity
//            cv::Scalar intensity_i = cv::sum(listImages[i]);
//            cv::Scalar intensity_j = cv::sum(listImages[j]);
//            if ( intensity_i[1] >intensity_j[1] )
//            {
//               listImages[j].copyTo(listImages[i]);
//               tmp.copyTo(listImages[i]);
//            }
        }
    }
    /// We Save the Minimum - ie 1st element - As the background - so any moving components are removed
    bgFrame = listImages[0]; //
    assert(!bgFrame.empty());

   // For Debuging
    cv::imshow("BG Model",listImages[1]);
   #ifdef _DEBUG
    cv::imshow("Median BG Model image",listImages[listImages.size() / 2]);
    cv::imshow("Lowest PX BG Model image",listImages[1]);
    cv::imshow("Highest PX BG Model image",listImages[listImages.size() -1]);
    #endif

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
//        if (fishBearingRads < 0)
//            fishBearingRads  += 2.0*CV_PI;

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



void trackerState::writeFishDataCSVHeader(QFile& data)
{

    /// Write Header //
    QTextStream output(&data);
    output << "frameN\ttailLengthpx\tThetaSpine_0\t";
    for (int i=1;i<FishTailSpineSegmentCount;i++)
        output <<  "DThetaSpine_" << i << "\t";

    output << "\n";
}


bool trackerState::openDataFile()
{
    int Vcnt = 1;
    bool newFile = false;
    //Make ROI dependent File Name
    QString outcvsfilename;
    //data.setFileName(dirOutPath);
    //Make Sure We do not Overwrite existing Data Files

    while (!newFile)
    {
        if (!outdatafile.exists() || outdatafile.isOpen()) //Write HEader
        {
            newFile = true;
        }else{ //File Exists
            //Append Sequence Number to Output File name
          //Increase Seq Number And Reconstruct Name
            Vcnt++;
            // If postfix (track / food) already there, then just add new number
            QFileInfo outInfo(outdatafile);
            outcvsfilename = outInfo.path() + "/" + outInfo.baseName().append("_"+QString::number(Vcnt) + "." + outInfo.suffix() ) ;
            outdatafile.setFileName(outcvsfilename );
            }
    }

    /// Open File for Appending
    if (!outdatafile.open(QFile::WriteOnly |QFile::Append))
    {

        std::cerr << "Could not open output file : " << outdatafile.fileName().toStdString() << std::endl;
        return(false);
    }else {
        //New File
        writeFishDataCSVHeader(outdatafile);
        std::clog << "Create new file " << outdatafile.fileName().toStdString() << " for track data." << std::endl;
    }

    return (true);
}

void trackerState::closeDataFile()
{
    outdatafile.close();
    std::clog << " Closed Output File " << outdatafile.fileName().toStdString() << std::endl;
    lastError.first = QString(" Closed Output File :") + outdatafile.fileName();
    lastError.second = 0;
}
