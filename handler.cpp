#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <iostream>
#include <sstream>
#include<fstream>

#include "handler.h"

using namespace std;
using namespace cv;

float angleBetween(const cv::Point2f &v1, const cv::Point2f &v2)
{
    float len1 = sqrt(v1.x * v1.x + v1.y * v1.y);
    float len2 = sqrt(v2.x * v2.x + v2.y * v2.y);

    float dot = v1.x * v2.x + v1.y * v2.y;

    float a = dot / (len1 * len2);

    if (a >= 1.0)
        return 0.0;
    else if (a <= -1.0)
        return CV_PI;
    else
        return acos(a); // 0..PI
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

    trackerState.initInputVideoStream();
    t_tracker_error lastError = trackerState.getLastError();
    window_main.LogEvent(lastError.first,lastError.second);


    window_main.LogEvent(QString("Total frames to track:") + QString::number( trackerState.ImageSequenceProvider()->getTotalFrames() ),5 );
    window_main.LogEvent(QString("Calculating Background image"),10 );
    window_main.setBusyOn();
    trackerState.initBGSubstraction();
    window_main.setBusyOff();

    //Once BG Donw, Rewind Video
    trackerState.ImageSequenceProvider()->setCurrentFrameNumber(trackerState.startFrame);

    //read input data. ESC or 'q' for quitting
    while(!trackerState.bExiting && !trackerState.atLastFrame())
    {
        /// Flow Control Code  - For When Looking at Specific Frame Region ///
        // 1st Check If user changed Frame - and go to that frame
        nFrame = trackerState.ImageSequenceProvider()->getCurrentFrameNumber();



        //if (trackerState.bPaused || trackerState.bStartFrameChanged)
        //    continue;
     //        trackerState.bStartFrameChanged = false; //Reset

        frame = trackerState.getNextFrame();
        if (frame.empty())
        {
            t_tracker_error lastError = trackerState.getLastError();
            window_main.LogEvent(lastError.first,lastError.second);
        }

        //Update Frame Index
        nFrame = trackerState.ImageSequenceProvider()->getCurrentFrameNumber();


        //Check If StopFrame Reached And Pause
        if (trackerState.atStopFrame() && !trackerState.bPaused)
        {
             trackerState.bPaused = true; //Stop Here
             window_main.LogEvent(QString(">>Stop Frame Reached - Video Paused<<"),5);
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
            trackerState.tailsplinefit = fitSpineToIntensity(frame_denoise,trackerState,trackerState.tailsplinefit);
            trackTailOpticFlow(frame_denoise,lastframe,trackerState.tailsplinefit,nFrame,trackerState.tailsplinefit);
            drawSpine(outframe,trackerState,trackerState.tailsplinefit );
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
            trackerState.fishBearingRads = angleBetween(cv::Point(trackerState.tailsplinefit[1].x,trackerState.tailsplinefit[1].y),
                                                    trackerState.ptTailRoot);

            trackerState.FishTailSpineSegmentLength = cv::norm(trackerState.ptTailRoot - cv::Point(trackerState.tailsplinefit[1].x,trackerState.tailsplinefit[1].y))/trackerState.FishTailSpineSegmentCount;
            trackerState.initSpine();
            trackerState.FitTailConfigState = 0;
        }


        // Display video Image on GUI
        window_main.showCVImage(outframe, nFrame);

        lastframe = frame_denoise; //Save

         QCoreApplication::processEvents(QEventLoop::AllEvents);
     }/// Main While - Reading Frames Loop


    return (trackerState.ImageSequenceProvider()->getCurrentFrameNumber());
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


t_fishspline fitSpineToIntensity(cv::Mat &frameimg_Blur,trackerState& trackerState,t_fishspline spline){

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

        angle = spline[k-1].angleRad/CV_PI*180.0-90.0; //Get Angle In Degrees for Arc Drawing Tranlated Back to 0 horizontal
        //Construct Elliptical Circle around last Spine Point - of Radius step_size
        cv::ellipse2Poly(cv::Point(spline[k-1].x,spline[k-1].y),
                cv::Size(step_size,step_size), 0, angle-trackerState.FitTailIntensityScanAngleDeg, angle+trackerState.FitTailIntensityScanAngleDeg, 1, ellipse_pts);

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
        for(int idx=0;idx<(int)ellipse_pts.size();idx++){
            //Obtain Value From Image at Point on Arc - Boundit in case it goes outside image
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

        //Update Spline to COM (Centre Of Mass) And Set As New Spline Point
        uint comIdx = iTailArcMoment/iSumPxIntensity;
        spline[k].x     = ellipse_pts[iMaxIdx].x;
        spline[k].y     = ellipse_pts[iMaxIdx].y;
        /// Get Arc tan and Translate back to 0 being the Vertical Axis
        if (k==1) //1st point Always points in the opposite direction of the body
            spline[k-1].angleRad    = (trackerState.fishBearingRads)-CV_PI;// ; //  //Spine Looks In Opposite Direction
        else
            //angleBetween(cv::Point(spline[k].y,spline[k].x),cv::Point(spline[k-1].y,spline[k-1].x))+CV_PI/2.0;
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

        //Connect Spine Points
        cv::line(outFrame,cv::Point(spline[j].x,spline[j].y),
                 cv::Point(spline[j+1].x,spline[j+1].y),CV_RGB(120,120,0),1);

            //cv::line(outFrame,cv::Point(spline[j].x,spline[j].y),cv::Point(spline[j+1].x,spline[j+1].y),TRACKER_COLOURMAP[0],1);
//        else
//        { //Draw Terminal (hidden) point - which is not a spine knot
//            cv::Point ptTerm;
//            ptTerm.x = spline[j].x + ((double)c_spineSegL)*sin(spline[j].angleRad);
//            ptTerm.y = spline[j].y - ((double)c_spineSegL)*cos(spline[j].angleRad);

//            cv::line(outFrame,cv::Point(spline[j].x,spline[j].y),ptTerm,TRACKER_COLOURMAP[0],1);
//        }
    }
    //Draw Final Tail (imaginary Spine) Point
    //if (inactiveFrames == 0)
    //    cv::circle(outFrame,cv::Point(spline[c_spinePoints-1].x,spline[c_spinePoints-1].y),2,TRACKER_COLOURMAP[c_spinePoints-1],1);
    //else
        cv::circle(outFrame,cv::Point(spline[c_spinePoints-1].x,spline[c_spinePoints-1].y),2,CV_RGB(100,100,100),1);

}




void mouse_GetVector(int event, int x, int y, int flags, void* p){
    mouse_GetVector_param* param=(mouse_GetVector_param*)p;
	if  ( event == cv::EVENT_LBUTTONDOWN ){
	    param->pt1.x=x;
		param->pt1.y=y;
		param->status=true;
    } 
    
	if (param->status==true){
		param->pt2.x=x;
		param->pt2.y=y;
	}

    if (event==cv::EVENT_LBUTTONUP ){
            param->status=false;
    }
	   
}

void get_interp(Mat &src, Point2i start, Point2d tgt, vector<Point2i>& anchor_pts,Mat mask){
	MatIterator_<uchar> it_mask;
	MatIterator_<float> it=src.begin<float>();
	int counter=0;
	int x,y;
	int cols=src.cols;
	double av_x=0, av_y=0, weight=1;
	tgt.x/=tgt.dot(tgt);
	tgt.y/=tgt.dot(tgt);
	Point2i pos=start;
	int n_anch=0;

	while(n_anch<20){
		Mat mask2 = Mat::zeros(mask.rows,mask.cols,CV_8U);
        
		circle(mask2,pos,6,255,1);
		src.copyTo(mask,mask2);
		it=src.begin<float>();
		counter=0;
		av_x=0; av_y=0; weight=1;
		for(it_mask=mask2.begin<uchar>();it_mask!=mask2.end<uchar>();++it_mask){
			y=counter/cols;
			x=counter%cols;
			if(*it_mask>0) {
				if((x-pos.x)*tgt.x+(y-pos.y)*tgt.y>0){
				//if((x-pos.x)*tgt.x+(y-pos.y)*tgt.y>sqrt(pow(x-pos.x,2)+pow(y-pos.y,2))*sqrt(tgt.dot(tgt))*0.6){
					if(*it<weight) {
						av_x=x;
						av_y=y;
						weight=*it;
					}
				}
			}
			++it;
			counter++;
		}
		anchor_pts.push_back(Point2i(floor(av_x),floor(av_y)));
		tgt=anchor_pts.back()-pos;
		tgt.x/=tgt.dot(tgt);
		tgt.y/=tgt.dot(tgt);
		
		pos=anchor_pts.back();
		n_anch++;
	}

}

void get_interp2(Mat &src, Point2i start, Point2d tgt_start, double step_size, vector<Point2i>& anchor_pts){
	const size_t AP_N=18;
	vector<Point2i> tmp_pts(AP_N);
	Point2i tgt;
	anchor_pts.resize(AP_N);
	anchor_pts[0]=start;
	tmp_pts[0]=start;
	unsigned int k=0;
	double loc,loc_add, val=0;
	int angle;
	srand(2);
	ofstream out("file.log");
	
	for(unsigned int counter=0;counter<7000;counter++){
		loc=0;
		for(k=1;k<AP_N;k++){			
			vector<Point2i> ellipse_pts;
			if(k==1) tgt=tgt_start;
			else tgt=tmp_pts[k-1]-tmp_pts[k-2];
			
			if(tgt.x>0 ){
				angle=floor(atan((double)tgt.y/tgt.x)/CV_PI*180);
			} else if(tgt.x<0) {
				angle=180+floor(atan((double)tgt.y/tgt.x)/CV_PI*180);
			} else if(tgt.x==0){
				if(tgt.y>0) angle=90;
				else angle=-90;
			}

			ellipse2Poly(tmp_pts[k-1], Size(4,4), 0, angle-20, angle+20, 1, ellipse_pts);
					    
			int index;
			loc_add=0;
			while(loc_add==0){
				index=rand() % ellipse_pts.size();
				loc_add=src.at<float>(ellipse_pts[index].y,ellipse_pts[index].x);
			}
			tmp_pts[k]=ellipse_pts[index];
			out<<tgt<<' '<<angle<<' '<<tmp_pts[k]<<' '<<loc<<' '<<index<<' '<<ellipse_pts.size()<<endl;
			loc+=loc_add;

			//if(loc>val) break;
		}
	    
		if(loc > val){
			val=loc;
			for(k=1;k<AP_N;++k) anchor_pts[k]=tmp_pts[k];
		}
		out<<endl;
	}
}


void get_interp3(Mat &src, Point2i start, Point2d tgt_start, double step_size, vector<Point2i>& anchor_pts){
	const size_t AP_N=20;
	vector<Point2i> tmp_pts(AP_N);
	Point2i tgt;
	anchor_pts.resize(AP_N);
	anchor_pts[0]=start;
	tmp_pts[0]=start;
	unsigned int k=0;
	double loc,loc_add, val=0;
	int angle;
	ofstream out("file.log");
	
	for(k=1;k<AP_N;k++){			
		vector<Point2i> ellipse_pts;
		if(k==1) tgt=tgt_start;
		else tgt=tmp_pts[k-1]-tmp_pts[k-2];
		
		if(tgt.x>0 ){
			angle=floor(atan((double)tgt.y/tgt.x)/CV_PI*180);
		} else if(tgt.x<0) {
			angle=180+floor(atan((double)tgt.y/tgt.x)/CV_PI*180);
		} else if(tgt.x==0){
			if(tgt.y>0) angle=90;
			else angle=-90;
		}
		
		ellipse2Poly(tmp_pts[k-1], Size(4,4), 0, angle-20, angle+20, 1, ellipse_pts);
		
		int index;
		loc_add=0;
		index=0;
		for(index=0;index<ellipse_pts.size();++index){
			loc=src.at<float>(ellipse_pts[index].y,ellipse_pts[index].x);
			if(loc>loc_add){
				tmp_pts[k]=ellipse_pts[index];
				loc_add=loc;
			}
		}
		//out<<tgt<<' '<<angle<<' '<<tmp_pts[k]<<' '<<loc<<' '<<index<<' '<<ellipse_pts.size()<<endl;
	    
		anchor_pts[k]=tmp_pts[k];
	}
	out<<endl;
}










