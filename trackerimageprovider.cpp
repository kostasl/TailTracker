#include "trackerimageprovider.h"


cv::Mat trackerImageProvider::getNextFrame()
{
    cv::Mat nextFrame;

    ///READ FRAME - Check For Error
     try //Try To Read The Image of that video Frame
    {
        //read the current frame
        if (inputSourceMode == sourceVideoTypes::ImageSequence)
        {
            //Check if more file  available /If We have not emptied the file list of files

            if (imageSequenceFiles.count() > 0){
                pcvcapture->open(imageSequenceFiles.first().filePath().toStdString());
                std::clog << imageSequenceFiles.first().filePath().toStdString() <<std::endl;
                imageSequenceFiles.pop_front();
            }
        }

        if(!pcvcapture->read(nextFrame))
        {/// Frame - Can't Read Next Frame
            if (atFirstFrame())
            {
                //window_main.LogEvent("# [Error]  Unable to read first frame.",2);
                lastError.first = "[ERROR]  Unable to read first frame.";
                lastError.second = 2;
                currentFrameNumber = 0; //Signals To caller that video could not be loaded.

                setCurrentFrameNumber(currentFrameNumber+1); //Skip Frame
                //Delete the Track File //
                //removeDataFile(outdatafile);
                //exit(EXIT_FAILURE);
            }
            else //Not Stuck On 1st Frame / Maybe Vid Is Over?>
            {
               if (!atLastFrame())
               {
                   //window_main.LogEvent("[Error] Stopped Tracking before End of Video - Delete Data File To Signal its Not tracked",3);
                   //removeDataFile(outdatafile); //Delete The Output File
                   lastError.first = "[Error] Failed to read frame " + QString::number(currentFrameNumber) + ".Skipping";
                   lastError.second = 2;
                   setCurrentFrameNumber(currentFrameNumber+1); //Skip Frame
               }
               else { ///Reached last Frame Video Done
                   //window_main.LogEvent(" [info] processVideo loop done!",5);
                   lastError.first = "[INFO] processVideo loop done!";
                   lastError.second = 5;
                   //::saveImage(frameNumberString,QString::fromStdString( gTrackerState.gstroutDirCSV),videoFilename,outframe);
                   totalVideoFrames = currentFrameNumber;
                   assert(atLastFrame());
               }
               //continue;
           }
        }else //Frame read Returned ok
            currentFrameNumber++;

    }catch(const std::exception &e) //Error Handling
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
    {
        /// Do Simple Brightness Contrast Transform //
        // Change Brightness Contrast
        nextFrame.convertTo(nextFrame, -1, contrastGain, brightness);
        //nextFrame = frame_BCTrans;

        nextFrame.copyTo(currentFrame);
    }

    //Set It Internally So View is updated
    setNextFrame(nextFrame);


    return(nextFrame);

}

bool trackerImageProvider::atFirstFrame(){
    return(currentFrameNumber == 1);
}

bool trackerImageProvider::atLastFrame(){
    return(currentFrameNumber == (totalVideoFrames));
}

// TODO: allow for stopping at user provided endFrame
bool trackerImageProvider::atStopFrame(){
    return(currentFrameNumber == (totalVideoFrames));
}

uint trackerImageProvider::getTotalFrames(){
 return(totalVideoFrames);
}

uint trackerImageProvider::endFrameNumber(){

    if (imageSequenceFiles.count() > 0)
           endFrame = imageSequenceFiles.last().baseName().toInt();

return (endFrame);
}


uint trackerImageProvider::getCurrentFrameNumber(){

    if (inputSourceMode == sourceVideoTypes::VideoFile) //Ony for Videos attempt to update FrameNumber
        currentFrameNumber = pcvcapture->get(CV_CAP_PROP_POS_FRAMES);
    else if (imageSequenceFiles.count() > 0)
        currentFrameNumber = imageSequenceFiles.first().baseName().toInt();

    return(currentFrameNumber);
}


void trackerImageProvider::setCurrentFrameNumber(uint nFrame)
{
    if (inputSourceMode == sourceVideoTypes::VideoFile)
        pcvcapture->set(CV_CAP_PROP_POS_FRAMES,nFrame);
    else{
        ///Reload Image File List
        imageSequenceFiles  = videoFile.dir().entryInfoList(QDir::Filter::Files);
        ///Scan for Desired Current Frame
        while (imageSequenceFiles.count() > 0)
        {   //Search for Frame Number in File Name
            if (imageSequenceFiles.first().baseName().toInt() == nFrame)
                break; //Found File With Set frame Number
            else
                imageSequenceFiles.pop_front();
        }//While desired filename with nFrame number has not been found
    }

    currentFrameNumber = nFrame;
}

cv::Mat trackerImageProvider::getCurrentFrame()
{
    return (currentFrame);
}


void trackerImageProvider::closeInputStream()
{
    if (!pcvcapture)
        return;

    if (pcvcapture->isOpened())
        pcvcapture->release();
}


//Return Last Recorded error and Remove error
t_tracker_error trackerImageProvider::getLastError()
{
  t_tracker_error tmp_lastError = lastError;
  //Once Last is Retrieved,  Set next Error To OK
  lastError.first = "OK";
  lastError.second = 10; //Lowest Priority
  return(tmp_lastError);
}

int trackerImageProvider::initInputVideoStream(QString filename)
{
    videoFile.setFile( filename);
    initInputVideoStream(videoFile);
}

/// \returns 0 if error, error retrieved by calling getLastError()
int trackerImageProvider::initInputVideoStream(QFileInfo pvideoFile)
{

    videoFile = pvideoFile;

    int ret = 0;
    if (!videoFile .exists())
    {
        lastError.first = "[ERROR] Failed to find video file";
        lastError.second = 1;
        ret = 0;
    }else
    {
        lastError.first = QString("Processing video file") + videoFile.fileName();
        lastError.second = 9;
        ret = 1;
    }

    //OPEN IMAGE SEQUENCE
    if (videoFile.suffix() == "pgm" || videoFile.suffix() == "tiff" )
    {
        inputSourceMode = sourceVideoTypes::ImageSequence;
        videoFile.dir().setSorting(QDir::SortFlag::Name);
        imageSequenceFiles  = videoFile.dir().entryInfoList(QDir::Filter::Files);
        videoFile           = imageSequenceFiles.first();

        QString filenamePatt = QString("/%") + (QString::number(videoFile.baseName().length())) + "d." + videoFile .suffix();
        std::string cvFilePattern = QString(videoFile.path() + filenamePatt).toStdString();

        //This is likely to fail as we do not start from 0
        std::clog << "Attempting to open image sequence " << cvFilePattern << std::endl;

        pcvcapture = new cv::VideoCapture( videoFile.filePath().toStdString() );
        startFrame = ( videoFile.baseName().toInt() );
        currentFrameNumber =startFrame;

        pcvcapture->open(cvFilePattern);
        pcvcapture->set(CV_CAP_PROP_POS_FRAMES, currentFrameNumber );

        if(!pcvcapture->isOpened())
        {
            //error in opening the video input
            lastError.first = "[ERROR] Failed to open images ";
            lastError.second = 1;
            ret = 0;
        }else{
            lastError.first = "[INFO] image sequence started";
            lastError.second = 10;
            ret = 1;
        }

        vidfps = 470;
        totalVideoFrames =  videoFile.dir().count();

     }//If File Sequence

    /// Open Single video file
    if (videoFile.suffix() == "avi" || videoFile.suffix() == "mp4" || videoFile.suffix() == "mkv")
    {
        inputSourceMode = sourceVideoTypes::VideoFile;
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

        vidfps = (pcvcapture->get(CV_CAP_PROP_FPS) );
        totalVideoFrames = (pcvcapture->get(CV_CAP_PROP_FRAME_COUNT) );
    }//If Video File

 return (ret);
}

void trackerImageProvider::setNextFrame(QPixmap frm)
{

    pixmap = frm.copy(0,0,frm.width(),frm.height());// const //cv::Mat::zeros(200,200,CV_8U ); //frm.clone();
}

//Called By getNext Frame
void trackerImageProvider::setNextFrame(cv::Mat frm)
{

    if (!frm.empty())
        pixmap = QPixmap::fromImage(QImage((unsigned char*) frm.data, frm.cols, frm.rows,
                                                         QImage::Format_RGB888));
}


//Id Could Be Frame Number If We Add Frames to An Array
QPixmap trackerImageProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
      int width = 200;
      int height = 200;

    //QPixmap pixmap;
    //qDebug() << "Image with id" << id << "is requested";

    if (!pixmap.size().width() > 100)
    {
     //QPixmap  pixmap = QPixmap::fromImage(QImage((unsigned char*) currentFrame.data, currentFrame.cols, currentFrame.rows,
     //                                         QImage::Format_RGB888));
       //make temp
        QPixmap pxMapempty(requestedSize.width() > 0 ? requestedSize.width() : width,
                       requestedSize.height() > 0 ? requestedSize.height() : height);
        pxMapempty.fill(QColor("blue").rgba());
        pixmap = pxMapempty;
    }else
   // {

   if (size)
      *size = QSize(pixmap.width(), pixmap.height());

   //pixmap(requestedSize.width() > 0 ? requestedSize.width() : width,
   //               requestedSize.height() > 0 ? requestedSize.height() : height);
   //pixmap.fill(QColor("blue").rgba());
    //}

 return pixmap;
}
