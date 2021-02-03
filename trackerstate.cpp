#include "trackerstate.h"

using namespace std;

trackerState::trackerState()
{

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
trackerState::trackerState(cv::CommandLineParser& parser)
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
