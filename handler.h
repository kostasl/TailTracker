#include <iostream>
#include <sstream>
#include<fstream>


#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QDirIterator>
#include <QDir>
#include <QTextObject>
#include <QTextItem>
#include <QtQuick>
#include <QDebug>
#include <QQmlComponent>
//#include <QThread>
#include <QTime>
#include <QElapsedTimer>

#include <trackerstate.h>
#include <trackerimageprovider.h>
#include <mainwindow.h>

using namespace std;
using namespace cv;

class mouse_GetVector_param {
	public:
	Point2i pt1, pt2;
	bool status;
};

class mainwindow;
class trackerState;

void mouse_GetVector(int event, int x, int y, int flags, void* p);
void get_interp(Mat &src, Point2i start, Point2d tgt,vector<Point2i>& anchor_pts,Mat mask);
void get_interp2(Mat &src, Point2i start, Point2d tgt,double, vector<Point2i>& anchor_pts);
void get_interp3(Mat &src, Point2i start, Point2d tgt,double, vector<Point2i>& anchor_pts);
unsigned int processVideo(mainwindow& window_main, trackerState& trackerState);
t_fishspline fitSpineToIntensity(cv::Mat &frameimg_Blur,trackerState& trackerState,t_fishspline spline);
void drawSpine(cv::Mat& outFrame,trackerState& trackerState,t_fishspline& spline);
float angleBetween(const cv::Point &v1, const cv::Point &v2);
