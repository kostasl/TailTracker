#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <iostream>
#include <sstream>
#include<fstream>

#include "handler.h"

using namespace std;
using namespace cv;

cv::Mat frame_debug;
float angleBetween(const cv::Point2f &v1, const cv::Point2f &v2)
{
    // ReCalc Angle in 0 - 2PI range Of previous Spline POint to this New One
    return(std::atan2(v1.y-v2.y,v1.x-v2.x))+CV_PI/2.0;

//    float len1 = sqrt(v1.x * v1.x + v1.y * v1.y);
//    float len2 = sqrt(v2.x * v2.x + v2.y * v2.y);

//    float dot = v1.x * v2.x + v1.y * v2.y;

//    float a = dot / (len1 * len2);

//    if (a >= 1.0)
//        return 0.0;
//    else if (a <= -1.0)
//        return CV_PI;
//    else
//        return acos(a); // 0..PI
}


//
// Process Larva video, removing BG, detecting moving larva- Setting the learning rate will change the time required
// to remove a pupa from the scene -
//
unsigned int processVideo(mainwindow& window_main, trackerState& trackerState)
{
    //cv::namedWindow("Trackerdisplay",cv::WINDOW_NORMAL | cv::WINDOW_KEEPRATIO);
    //cv::resizeWindow("Trackerdisplay",250,250);

    QElapsedTimer otLastUpdate; //Time Since Last Progress Report
    otLastUpdate.start();

    cv::Mat frame,outframe,lastframe;
    unsigned int nFrame         = 0;
    unsigned int nErrorFrames   = 0;

    QString frameNumberString;
    //OpenCV open The video File

    /// Do checks for param and initialize inputStream
    if (!trackerState.isReady())
        return 1; //Wait Until Ready
    else
        trackerState.initInputVideoStream(); //Draws The 1st file from List

    t_tracker_error lastError = trackerState.getLastError();
    window_main.LogEvent(lastError.first,lastError.second);
    window_main.LogEvent(QString("Total frames to track:") + QString::number( trackerState.ImageSequenceProvider()->getTotalFrames() ),5 );
    window_main.LogEvent(QString("Calculating Background image"),10 );

    frame = trackerState.getNextFrame();
    window_main.showCVImage(frame, trackerState.getCurrentFrameNumber());

    /// Do BG Substraction
    window_main.setBusyOn();
    trackerState.initBGSubstraction();
    window_main.setBusyOff();




     //Once BG Donw, Rewind Video
    trackerState.ImageSequenceProvider()->setCurrentFrameNumber(trackerState.startFrame);

    /// Open Data File
    trackerState.openDataFile();

    //read input data. ESC or 'q' for quitting
    while(!trackerState.bExiting && !trackerState.atLastFrame())
    {
        /// Flow Control Code  - For When Looking at Specific Frame Region ///
        // 1st Check If user changed Frame - and go to that frame
        nFrame = trackerState.ImageSequenceProvider()->getCurrentFrameNumber();

        /// Get Next Frame Image from source
        frame = trackerState.getNextFrame();
        t_tracker_error lastError = trackerState.getLastError();
        if (lastError.second != 0) //Check If Critical Error- Break from loop
        {
            window_main.LogEvent(lastError.first,lastError.second);
            if (lastError.second == 1)
                break;
            else
                continue;
        }

        //Update Frame Index
        nFrame = trackerState.ImageSequenceProvider()->getCurrentFrameNumber();


        //Check If StopFrame Reached And Pause
        if (trackerState.atStopFrame() && !trackerState.bPaused)
        {
             trackerState.bPaused = true; //Stop Here
             window_main.LogEvent(QString(">> Stop Frame Reached - Video Paused <<"),5);
        }

         //Pause on 1st Frame If Flag Start Paused is set
         if (trackerState.bStartPaused && trackerState.atFirstFrame() && !trackerState.bPaused)
         {
             trackerState.bPaused = true; //Start Paused //Now Controlled By bstartPaused
             window_main.LogEvent(QString("[info]>> Video Paused<<"),5);
         }

         /// Start Processing The Frame

        /// IMAGE TRANSFORMS Experimental ///
        cv::Mat abs_dst,frame_blur,frame_denoise,frame_Ledge,frame_BCTrans;

        //Copy Frame
        outframe = frame.clone();
       //Process And Track  Tail
        cv::putText(outframe, cv::String(QString::number(nFrame).toStdString()), cv::Point(140, 20),
                cv::FONT_HERSHEY_SIMPLEX, 0.6 , cv::Scalar(250,250,0));

        // Reduce noise by blurring with a Gaussian filter ( kernel size = 3 )
        GaussianBlur( frame, frame_blur, Size(3, 3), 0, 0, BORDER_DEFAULT );
        cvtColor( frame_blur, frame_blur, COLOR_BGR2GRAY ); // Convert the image to grayscale

        //cv::fastNlMeansDenoising(frame_blur, frame_denoise,3.0,7, 21);
        frame_denoise = frame_blur;

        //BG
        cv::Mat fgMask,fgFrame,bgFrame;
        cv::Mat frame_diff  = cv::Mat(frame_denoise.size(), CV_16SC1);
        //bgFrame = cv::Mat(frame_denoise.size(), frame_denoise.type());
        //trackerState.pBGsubmodel->getBackgroundImage(bgFrame);
        trackerState.bgFrame.copyTo(bgFrame);
        cvtColor( bgFrame,  bgFrame, COLOR_BGR2GRAY ); // Convert the image to grayscale
        //cvtColor( frame_denoise, frame_denoise, CV_8UC1 ); // Convert the image to grayscale

        bgFrame.convertTo(bgFrame,-1,0.8,-1);
        cv::absdiff(frame_denoise, bgFrame,frame_diff );
        frame_diff.copyTo(frame_denoise);

#if _DEBUG
        cv::imshow("BG Image",bgFrame);
        cv::imshow("Denoised And Blured And B&C Increased", frame_denoise);
        cv::imshow("Substracted Denoised And Blured And B&C Increased", frame_denoise);
#endif

        //trackerState.pBGsubmodel->getBackgroundImage(bgFrame);
        /// Handle TAIL Spine Initialization and Fitting
        /// Main Method Uses Pixel Intensity //
        /// Secondary Method Uses Optic Flow
        if (trackerState.FitTailConfigState == 0)
        {
#ifdef _DEBUG
            outframe.copyTo(frame_debug);
#endif
            trackerState.tailsplinefit = fitSpineToIntensity(frame_denoise,trackerState,trackerState.tailsplinefit);
            //trackTailOpticFlow(frame_denoise,lastframe,trackerState.tailsplinefit,nFrame,trackerState.tailsplinefit);
            drawSpine(outframe,trackerState,trackerState.tailsplinefit );

            trackerState.saveFrameTrack();
        }

        if (trackerState.FitTailConfigState == 1) //Making The Arrow
        {
           cv::arrowedLine(outframe,
                           trackerState.ptTailRoot,
                           cv::Point(trackerState.tailsplinefit[1].x,trackerState.tailsplinefit[1].y),
                           CV_RGB(200,10,10), 2);
           //Circle Showing Tail Root
           cv::circle(outframe,trackerState.ptTailRoot,trackerState.FishTailSpineSegmentLength,CV_RGB(20,100,100),2);
        }
        //Mouse Cursor On Image Coordinate
        cv::circle(outframe,cv::Point(window_main.mouseX,window_main.mouseY),
                   2,CV_RGB(220,180,120),2);


        // Done Setting Spine Direction - Set Length And Bearing From User Input
        if (trackerState.FitTailConfigState == 2)
        {
            trackerState.fishBearingRads = angleBetween(trackerState.ptTailRoot,cv::Point(trackerState.tailsplinefit[1].x,trackerState.tailsplinefit[1].y)                                                    );

            trackerState.FishTailSpineSegmentLength = cv::norm(trackerState.ptTailRoot - cv::Point(trackerState.tailsplinefit[1].x,trackerState.tailsplinefit[1].y))/trackerState.FishTailSpineSegmentCount;
            trackerState.initSpine();
            trackerState.FitTailConfigState = 0;
        }


        // Display video Image on GUI
        window_main.showCVImage(outframe, nFrame);

        lastframe = frame_denoise; //Save

         QCoreApplication::processEvents(QEventLoop::AllEvents);
     }/// Main While - Reading Frames Loop

    trackerState.closeDataFile();

    if (trackerState.bExiting)
        return 0; //Stop The Main Loop
    else
    {//Done this loop - Set state to Not Ready and wait for instruction
        trackerState.bReady = false;
        return 1;
    }
}

//// Remove Pixel Noise
//if (gTrackerState.bRemovePixelNoise)
//        cv::fastNlMeansDenoising(frameImg_gray, frameImg_gray,2.0,7, 21);
////Update BG Model - If not stuck on same frame by being paused
//if (gTrackerState.bUseBGModelling)
//{
//  try{
//      pMOG2->apply(frameImg_gray,fgMOGMask,dLearningRate);
//      //
//  }catch(...)
//  {
//  //##With OpenCL Support in OPENCV a Runtime Assertion Error Can occur /
//  //In That case make OpenCV With No CUDA or OPENCL support
//  //Ex: cmake -D CMAKE_BUILD_TYPE=RELEASE -D WITH_CUDA=OFF  -D WITH_OPENCL=OFF -D WITH_OPENCLAMDFFT=OFF -D WITH_OPENCLAMDBLAS=OFF -D CMAKE_INSTALL_PREFIX=/usr/local
//  //A runtime Work Around Is given Here:
//      std::clog << "MOG2 apply failed, probably multiple threads using OCL, switching OFF" << std::endl;
//      pwindow_main->LogEvent("[Error] MOG2 failed, probably multiple threads using OCL, switching OFF");
//      cv::ocl::setUseOpenCL(false); //When Running Multiple Threads That Use BG Substractor - An SEGFault is hit in OpenCL
//  }
//}


t_fishspline fitSpineToIntensity(cv::Mat &frameimg_Blur,trackerState& trackerState,t_fishspline spline)
{

    const double Rad2Deg = (180.0/CV_PI);
    const size_t AP_N= trackerState.FishTailSpineSegmentCount;
    const int step_size = trackerState.FishTailSpineSegmentLength;
    const int c_gain = 100;
    //const int c_tailscanAngle = gFitTailIntensityScanAngleDeg;
    uint pxValMax;
    int angle; //In Deg of Where The spline point is looking towards - Used by Ellipse Arc Drawing

    //cv::Mat frameimg_Blur;
    //cv::GaussianBlur(imgframeIn,frameimg_Blur,cv::Size(5,5),5,5);
    //cv::imshow("IntensitTailFit",frameimg_Blur);

    std::vector<cv::Point> ellipse_pts; //Holds the Drawn Arc Points around the last spine Point

    for(unsigned int k=1;k<AP_N;k++)
    { //Loop Through Spine Points
        ellipse_pts.clear();

        angle = spline[k-1].angleRad/CV_PI*180.0-90.0;  //Get Angle In Degrees for Arc Drawing Tranlated Back to 0 horizontal
        //Construct Elliptical Circle around last Spine Point - of Radius step_size
        cv::ellipse2Poly(cv::Point(spline[k-1].x,spline[k-1].y),
                cv::Size(step_size,step_size),0, angle-trackerState.FitTailIntensityScanAngleDeg, angle+trackerState.FitTailIntensityScanAngleDeg, 1, ellipse_pts);

#ifdef _DEBUG
        cv::polylines(frame_debug,ellipse_pts,false,CV_RGB(250,250,0));
        cv::imshow("Arc",frame_debug);
#endif

        if (ellipse_pts.size() ==0)
        {
            qDebug() << "fitSpineToIntensity: Failed empty ellipse2Poly";
            continue;
        }
        ///Calculate Moment of inertia Sum m theta along arc
        pxValMax                = 0;
        int iMaxIdx            = 0; //Posiotn of Max Intensity Value
        uint iTailArcMoment     = 0;
        uint iPxIntensity       = 0;
        uint iSumPxIntensity    = 1;
        // Loop Integrate Mass //
        for(int idx=0;idx<(int)ellipse_pts.size();idx++)
        {
            //Obtain Value From Image at Point on Arc - Bound it, in case it goes outside image
            int x = std::min(frameimg_Blur.cols,std::max(1,ellipse_pts[idx].x));
            int y = std::min(frameimg_Blur.rows,std::max(1,ellipse_pts[idx].y));
            iPxIntensity = c_gain*frameimg_Blur.at<uchar>(cv::Point(x,y));

            //Use idx As Angle /Position
            iTailArcMoment  += idx*iPxIntensity;
            iSumPxIntensity += iPxIntensity;
            if (pxValMax < iPxIntensity)
            {
                pxValMax = iPxIntensity;
                iMaxIdx = idx;
            }
        } //Loop Through Arc Sample Points

        //Use Max Intensity Idx //
        //uint comIdx = iTailArcMoment/iSumPxIntensity;Update Spline to COM (Centre Of Mass) And Set As New Spline Point
        spline[k].x     = ellipse_pts[iMaxIdx].x;
        spline[k].y     = ellipse_pts[iMaxIdx].y;
        /// Get Arc tan and Translate back to 0 being the Vertical Axis
        if (k==1) //1st point Always points in the opposite direction of the body - But Avoid - angles that mess up Angle Diff when saving record
            spline[k-1].angleRad = (trackerState.fishBearingRads >=CV_PI)?(trackerState.fishBearingRads-CV_PI):(trackerState.fishBearingRads+CV_PI); //  //Spine Looks In Opposite Direction
        else
            spline[k-1].angleRad = std::atan2(spline[k].y-spline[k-1].y,spline[k].x-spline[k-1].x)+CV_PI/2.0; // ReCalc Angle in 0 - 2PI range Of previous Spline POint to this New One

        //Set Next point Angle To follow this one - Otherwise Large deviation Spline
        if (k < AP_N)
            spline[k].angleRad = spline[k-1].angleRad;

        //Constrain Large Deviations
        if (std::abs(spline[k-1].angleRad - spline[k].angleRad) > CV_PI/2.0)
           spline[k].angleRad = spline[k-1].angleRad; //Spine Curvature by Initializing next spine point Constraint Next
    }


    return(spline);
} //fitSpineToIntensity



/// Process Optic Flow of defined food model positions
/// Uses Lukas Kanard Method to get the estimated new position of Prey Particles
/// Note Skip First Tail Root Point - So  tail positions does not drift with OpticFlow
int trackTailOpticFlow(const cv::Mat frame_grey,const cv::Mat frame_grey_prev,t_fishspline spline,unsigned int nFrame,t_fishspline& spline_next )
{
    int retCount = 0;
   std::vector<cv::Point2f> vptSpine_current;
   std::vector<cv::Point2f> vptSpine_next;

   zftblobs vSpineKeypoints_current;
   zftblobs vSpineKeypoints_ret;

   std::vector<uchar> voutStatus;
   // L1 distance between patches around the original and a moved point, divided by number of pixels in a window, is used as a error measure.
   std::vector<float>    voutError;

   t_fishspline::iterator ft;

   splineKnotf pSpinePoint;
    //Fill POint Vector From Spine point vector
   int i = 1;
   for ( ft  = spline.begin(); ft!=spline.end(); ++ft)
   {
       //Skip Odd points (ie 1st ) and follow every 2nd point
       if (i%2 != 0 )
           continue;
       pSpinePoint = (splineKnotf)(*ft);

       cv::KeyPoint kptSpine(cv::Point2f(pSpinePoint.x,pSpinePoint.y),2,pSpinePoint.angleRad);
       vSpineKeypoints_current.push_back(kptSpine  );
       i++;
   }

    cv::KeyPoint::convert(vSpineKeypoints_current,vptSpine_current);

    //Calc Optic Flow for each food item
    if (vptSpine_current.size() > 0 && !frame_grey_prev.empty())
        cv::calcOpticalFlowPyrLK(frame_grey_prev,frame_grey,vptSpine_current,vptSpine_next,voutStatus,voutError,cv::Size(31,31),2);

    cv::KeyPoint::convert(vptSpine_next,vSpineKeypoints_ret);

    //Copy Over and Update Spine Positions - Based on Optic Flow
    spline_next = spline;
    //update spine Locations
      //Loop through new Key points
    for (int i=0;i<(int)vSpineKeypoints_ret.size();i++)
    {
        //Status numbers which has a value of 1 if next point is found,
        if (!voutStatus.at(i))
            continue; //ignore bad point
        cv::Point2f ptSpinePos_updated = vptSpine_next.at(i);
        spline_next[i].x = ptSpinePos_updated.x; //Update Spine Position X
        spline_next[i].y = ptSpinePos_updated.y; //Update Spine Position Y

    } //Check if Error
//
return retCount;
}



void drawSpine(cv::Mat& outFrame,trackerState& trackerState,t_fishspline& spline)
{
    int c_spinePoints = trackerState.FishTailSpineSegmentCount;

    for (int j=0; j<c_spinePoints-1;j++) //Rectangle Eye
    {
        //if (inactiveFrames == 0)
        //   cv::circle(outFrame,cv::Point(spline[j].x,spline[j].y),2,TRACKER_COLOURMAP[j],1);
        //else
        cv::circle(outFrame,cv::Point(spline[j].x,spline[j].y),2,CV_RGB(100,100,100),1);

        // Connect Spine Points
        cv::line(outFrame,cv::Point(spline[j].x,spline[j].y),
                 cv::Point(spline[j+1].x,spline[j+1].y),CV_RGB(120,120,0),1);

    }
    //Draw Final Tail (imaginary Spine) Point
    //if (inactiveFrames == 0)
    //    cv::circle(outFrame,cv::Point(spline[c_spinePoints-1].x,spline[c_spinePoints-1].y),2,TRACKER_COLOURMAP[c_spinePoints-1],1);
    //else
        cv::circle(outFrame,cv::Point(spline[c_spinePoints-1].x,spline[c_spinePoints-1].y),2,CV_RGB(100,100,100),1);

}











