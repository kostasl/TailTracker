#include "trackerstate.h"
#include "handler.h"

using namespace std;

trackerState::trackerState()
{


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

    InputFilefilters << "*.pgm" <<  "*.tiff" << "*.png" << "*.jpg";

    if (parser.has("help"))
    {
        parser.printMessage();
        return;
    }

 /// Check if vid file provided in arguments.
 /// If File exists added to video file list, otherwise save directory and open dialogue to choose a file from there
    if (parser.has("invideofile"))
    {   strvidFilename = QString::fromStdString( parser.get<std::string>("invideofile") );
        addinputFile(strvidFilename);
    }else {
        //QFileDialog::setFileMode(QFileDialog::Directory);
        //if (invidFileList.empty())
        //       invidFileList = QFileDialog::getOpenFileNames(nullptr, "Select videos or images to process", outdir.path() ,
       //                                                      "All std formats(*.png *.pgm *.avi *mp4);;Image sequences (*.tiff *.png *.jpg *.pgm);;Video files (*.mpg *.avi *.mp4 *.h264 *.mkv)", nullptr, nullptr);
        if (!invidFileList.isEmpty())
        {
            videodir.setPath(invidFileList.first());
            //invideofile = invidFileList.first(); /This is called in isReady
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
                    addinputFile(line);
            }
        }else
        {
            qWarning() << fvidfile.fileName() << " does not exist!";
        }
    }

    //Check if outdir provided otherwise open Dialog
    if (parser.has("outputdir"))
    {
        stroutFilename = QString::fromStdString(parser.get<std::string>("outputdir"));
        std::clog << "Output Data file : " << stroutFilename.toStdString() << std::endl;
        setOutputFile(stroutFilename);
    }
    else
    {
      // getting the filename (full path)
      //stroutFilename  = QFileDialog::getSaveFileName(nullptr, "Save tracks to output",invideofile.dir().path() +"/"+ invideofile.dir().dirName().append(".csv"), "CSV files (*.csv);", nullptr, nullptr);

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

     startFrame = parser.get<uint>("startframe");
     endFrame = parser.get<uint>("stopframe");

     MOGhistory =  parser.get<uint>("BGHistorySize");
     FishTailSpineSegmentCount =  parser.get<int>("spinepoints");

    ///* Create Morphological Kernel Elements used in processFrame *///
    kernelOpen          = cv::getStructuringElement(cv::MORPH_CROSS,cv::Size(1,1),cv::Point(-1,-1));
    kernelDilateMOGMask = cv::getStructuringElement(cv::MORPH_ELLIPSE,cv::Size(3,3),cv::Point(-1,-1));
    kernelOpenfish      = cv::getStructuringElement(cv::MORPH_ELLIPSE,cv::Size(3,3),cv::Point(-1,-1)); //Note When Using Grad Morp / and Low res images this needs to be 3,3
    kernelClose         = cv::getStructuringElement(cv::MORPH_ELLIPSE,cv::Size(5,5),cv::Point(-1,-1));

    mptrackerView = ptrackerView;
    /// create Gaussian Smoothing kernels for Contour //
    //assert(dGaussContourKernelSize % 2 == 1); //M is an odd number
    //getGaussianDerivs(dGaussContourKernelSigma,dGaussContourKernelSize,gGaussian,dgGaussian,d2gGaussian);


    initSpine();
}
/// END OF INIT GLOBAL PARAMS //


void trackerState::setCurrentFrameNumber(uint nFrame)
{
    bStartFrameChanged = (nFrame !=mptrackerView->getCurrentFrameNumber());
    bPaused = true;
    mptrackerView->setCurrentFrameNumber(nFrame);
}

uint trackerState::getCurrentFrameNumber()
{
    return (mptrackerView->getCurrentFrameNumber());
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

    datStream << Rad2Deg* tailsplinefit[0].angleRad << "\t" << tailsplinefit[0].x << "\t" << tailsplinefit[0].y;
    //Output Spine Point Angular Deviations from the previous spine/tail Segment in Degrees
    for (int i=1;i<(FishTailSpineSegmentCount-1);i++)
    {
       datStream << "\t" << Rad2Deg*(tailsplinefit[i-1].angleRad - tailsplinefit[i].angleRad);
       datStream << "\t" << tailsplinefit[i-1].x << "\t" << tailsplinefit[i-1].y;
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
            bReady = false;
            qDebug() << "User issued exit. ";

            exit(EXIT_SUCCESS);
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
        startTracking();
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
    QFileInfo videoFile = invideofile;

    if (videoFile.isDir())
       std::clog << "Reading image files from " << videoFile.absoluteFilePath().toStdString() << std::endl;

    if (invideofile.absoluteFilePath().contains(" ")){
        lastError.first = "File paths must not contain spaces ";
        lastError.second = 3;
        std::clog <<  lastError.first.toStdString()  << std::endl;
    }

    int ret    = mptrackerView->initInputVideoStream(videoFile);
    startFrame = mptrackerView->startFrame;
    lastError  = mptrackerView->getLastError();
    setCurrentFrameNumber(startFrame);

    //move to requested start frame
    mptrackerView->setCurrentFrameNumber(startFrame);
    qDebug() << "Start processing from frame #" << startFrame;

    if (endFrame == 0) //If User has not set Stop Point- Set it to end of video
        endFrame =  mptrackerView->endFrameNumber();


    assert(startFrame <= endFrame);
    //  Check If it contains no Frames And Exit
    if (mptrackerView->getTotalFrames() < 2)
    {
        lastError.first = "[ERROR] Next Video empty ";
        lastError.second = 2;
        ret = 0;
        mptrackerView->closeInputStream();
    }

    // Video  Paused upon opening
    if (bStartPaused)
        bPaused = true;

    //Show First Frame
    //mptrackerView->setNextFrame(getNextFrame());

    return ret;
}//End of Function



/// Do a prelim Run Through MOGHistory initial video frames and compute BG image
/// Save all BGImages on the way and then take the median image for the BG substraction
/// This BG image reduced in brightness and substracted from image before tracking the tail.
/// \brief Initialize BG substractor objects, depending on options / can use cuda
/// \todo check memory contraints - Avoid Crashing
void trackerState::initBGSubstraction()
{

    //Reset to initial value
    MOGLearningRate = c_MOGLearningRate;
    MOGhistory      = c_MOGhistory;
    //Cap Number of BG training Frames To not exceed numbe;r of available frames
    MOGhistory = (mptrackerView->getTotalFrames() < MOGhistory)?mptrackerView->getTotalFrames():MOGhistory;
    MOGLearningRate = max(MOGLearningRate,5.0/MOGhistory); //Adjust Learning Rate To Number of Available Frames

    if (MOGhistory == 0)
    {
        lastError.first = "[WARNING] No BG History size, so BG model not computed.";
        lastError.second = 4;
        return;
    }

    std::vector<cv::Mat> listImages(MOGhistory);
    std::vector<cv::Mat>::iterator lft;
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

        try{
            pBGsubmodel->getBackgroundImage(listImages[i++]);
        }catch(const std::bad_alloc &)
        {
            qDebug() <<  "[ERROR] getting BG Image " << i << ". Memory Allocation Error! Decrease MOGhistory";
            lft = listImages.begin();
            listImages.erase(lft);

        }

        //cv::imshow("fg MAsk Learning",fgMask);
        //Show Frame Being Processed
        mptrackerView->setNextFrame(listImages[i-1]);

        try{
             //an OpenGL context for  QSurfacerFormaIs  ERROR raised here on some systems
            //QCoreApplication::processEvents(QEventLoop::AllEvents);
            QCoreApplication::processEvents();
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

            if (listImages[i].empty())
                continue;

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
            QCoreApplication::processEvents();
        }
    }
    /// We Save the Minimum - ie 1st element - As the background - so any moving components are removed
    bgFrame = listImages[0]; //
    assert(!bgFrame.empty());

   // For Debuging
    cv::imshow("BG Model",listImages[1]);
    listImages.clear();
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
    }else
    {   //Empty No Next File Found
        invideofile = QFileInfo();
        videodir = invideofile.dir();


    }

    return(invideofile);
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
    output << "frameN\ttailLengthpx\tThetaSpine_0\t"<< "\t" "Spine_0_x\t"<< "\t" "Spine_0_y\t";
    for (int i=1;i<FishTailSpineSegmentCount;i++)
        output <<  "DThetaSpine_" << i << "\t" "Spine_" << i << "_x\t"<< "\t" "Spine_" << i << "_y\t";

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
    if (outdatafile.isOpen())
    {
        outdatafile.close();
        std::clog << " Closed Output File " << outdatafile.fileName().toStdString() << std::endl;
        lastError.first = QString(" Closed Output File :") + outdatafile.fileName();
        lastError.second = 0;

        // Done Processing This Video //
        // Invalidate File Processed
        lastvideofile = invideofile;
        invideofile.setFile("");
    }


}

bool trackerState::unloadCurrentVideo()
{
    if (invideofile.exists())
    {
        invideofile.setFile("");

        //setCurrentFrameNumber(0);
        mptrackerView->closeInputStream();
        startFrame = 0;
        endFrame = 0;
        closeDataFile();
        return(true);
    }else
        return(false);
}

bool trackerState::isReady()
{
    bool bReady = true;
    QString invideofileName;

    //Get Next File In the Queue and Init Stream
    //Sets invideo file to next.
       if(atLastFrame()) //Needs Rewind
       {
           bReady = false;
           lastError.first = "[Warning] Video finished tracking. Unload and closing data file.";
           lastError.second = 5;
           unloadCurrentVideo();
       }

//       if (invidFileList.isEmpty())
//       {
//           lastError.first = "[Error] No Files to track selected";
//           lastError.second = 2;
//           //bPaused = true;
//           bReady = false;
//       }
       if (!invideofile.exists() )
       {
           invideofile = getNextVideoFile();
           if (!invideofile.exists())
           {
               //qDebug() << "[Error] Need to select video file to process.";
               //qDebug() << "[Error] " << invideofileName << " Does not exist";
               lastError.first = "[Error] " + invideofile.fileName() + " Does not exist";
               lastError.second = 2;
               //bPaused = true;
               bReady = false;
            }else
           {
                  bReady = true;
                  bPaused = true;
           }


       }else{
            //Retain ref to output directory
            videodir.setPath(invideofile.dir().path());
            bReady = bReady & true;
        }

        if (outdatafile.fileName().length() < 5 )
        {
            //qDebug() << "[Error] Need to set output file to proceed.";
            lastError.first =  "[Error] Need to set output file to proceed.";
            lastError.second = 2;
            bPaused = true;
            bReady = bReady & true; //Needs to combine with previous check being true
        }



     return bReady;
}

bool trackerState::startTracking()
{
    if (atLastFrame())
    {
       lastError.first = "[INFO] Cannot upause - End of video reached ";
       lastError.second = 10;
       std::clog << "[INFO] Cannot upause - End of video reached " << std::endl;
       //mptrackerView->initInputVideoStream(invideofile);

       //mptrackerView->setCurrentFrameNumber(startFrame);
       if (invidFileList.count() > 0)
             std::clog << "[INFO] Proceed to next video" << std::endl;
        else
            std::clog << "[INFO] Need to select new video to process" << std::endl;

       return (false);
    }


    if (isReady())
    {
        qDebug() << "Tracker state : Unpaused:";
        bPaused = false;
        bStartPaused = false;
        bExiting = false;
        bReady = true;
    }

    return(true);
}
