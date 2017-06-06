#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <iostream>
#include <sstream>
#include<fstream>

using namespace std;
using namespace cv;

class mouse_GetVector_param {
	public:
	Point2i pt1, pt2;
	bool status;
};


void mouse_GetVector(int event, int x, int y, int flags, void* p);
void get_interp(Mat &src, Point2i start, Point2d tgt,vector<Point2i>& anchor_pts,Mat mask);
void get_interp2(Mat &src, Point2i start, Point2d tgt,double, vector<Point2i>& anchor_pts);
void get_interp3(Mat &src, Point2i start, Point2d tgt,double, vector<Point2i>& anchor_pts);
