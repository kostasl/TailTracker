#include<iostream>
#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include<sstream>
#include<fstream>
#include<string>
#include"../include/handler.h"
using namespace std;
using namespace cv;

int main(int argc, char* argv[]){
	int i=1505;
	int imax=atoi(argv[2]);
	string folder=argv[1];
	string prefix="";
	if(argc==4) prefix=argv[3];
    namedWindow("display1",cv::WINDOW_NORMAL | cv::WINDOW_KEEPRATIO);
	resizeWindow("display1",800,600);
    
	char c=' ';
	mouse_GetVector_param p; p.status=false;
    cv::setMouseCallback("display1",mouse_GetVector,&p);
	Mat draw;
    Mat kernel = (Mat_<float>(3,3) << 1,  1, 1,
	                                  1, -8, 1,
									  1,  1, 1);

	while(c!='q'){
		    stringstream ss;
			ss<<folder<<"/"<<prefix<<"0.tiff";
			Mat img=imread(ss.str().c_str(),IMREAD_UNCHANGED);
		    img.copyTo(draw);
			if(p.status) {
				arrowedLine(draw,p.pt1,p.pt2,2);
				circle(draw,p.pt2,12,2);
			}
			imshow("display1",draw);
			c=waitKey(10);
	}
    destroyWindow("display1");


    namedWindow("display2",cv::WINDOW_NORMAL | cv::WINDOW_KEEPRATIO);
	resizeWindow("display2",800,600);
	cvCreateTrackbar( "frame", "display2", &i, imax,  NULL);
	c='1';
	while(c!='q'){
		stringstream ss;
		ss<<folder<<'/'<<prefix<<i<<".tiff";
		Mat img=imread(ss.str().c_str(),IMREAD_UNCHANGED);
		Mat ones(img.rows,img.cols,CV_32F,Scalar(1));
		img.convertTo(img,CV_32F,1./255);
		vector<Point2i> a_pts;
		Point2d tangent;
		tangent=p.pt2-p.pt1;
		Mat draw, imgLaplacian,mask, draw_inv;
		GaussianBlur(img,draw,Size(5,5),5,5);
		filter2D(draw,imgLaplacian,CV_32F,kernel);
		threshold(imgLaplacian, mask,0,1,THRESH_BINARY);
		mask.convertTo(mask,CV_8U);
		draw_inv=ones-draw;
	    draw_inv.copyTo(draw_inv,mask);
		get_interp3(draw_inv,p.pt1,tangent,10,a_pts);
		
		for(unsigned int j=0;j<a_pts.size()-1;++j) line(draw,a_pts[j],a_pts[j+1],255,1);
	    imshow("display2",draw);
	    c=waitKey(10);
	}
  
	return 0;

}


