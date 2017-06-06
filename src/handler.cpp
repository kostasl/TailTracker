#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <iostream>
#include <sstream>
#include<fstream>
#include"../include/handler.h"

using namespace std;
using namespace cv;

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










