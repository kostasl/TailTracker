#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QApplication>
#include <QQmlProperty>
#include <QDebug>
#include <QDateTime>
#include <QObject>
#include <QPixmap>
#include <QImage>
#include <QQuickImageProvider>
#include <QMouseEvent>
#include <QKeyEvent>
//#include <QtOpencvCore>

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/core/core.hpp>
#include "opencv2/imgproc/imgproc.hpp"

#include <trackerimageprovider.h>

class mainwindow : public QObject
{
public:
    mainwindow(QQmlApplicationEngine& engine);
    void LogEvent(QString txt,int AlertLevel);
    void showCVImage(cv::Mat& img,uint nFrame );
    Q_OBJECT

   public slots:
       void cppSlot(const QString &msg) {
           qDebug() << "Called the cpp C++ slot with message:" << msg;
       }
       void mouseClickSlot() {
           mouseX = oMouseArea->property("mouseX").toInt();
           mouseY = oMouseArea->property("mouseY").toInt();
               qDebug() << "Mouse Click slot coords:" << mouseX << "," << mouseY;
       }
       void mouseDragSlot() {
           mouseX = oMouseArea->property("mouseX").toInt();
           mouseY = oMouseArea->property("mouseY").toInt();
               qDebug() << "Mouse move slot coords:" << mouseX << "," << mouseY;
       }


public:

cv::Mat frameScene; //CvMat Last Frame Drawn
QImage qimg; //SCene Image Updated in showCVImage Image
trackerImageProvider* ptrackerView;

protected:
    bool eventFilter(QObject *obj, QEvent *event);

private :
    QObject* txtLog;
    QObject* imgScene;
    QObject* oMouseArea; //Video frame Mouse Are
    int mouseX,mouseY; //mouse Position within The TrackerImage Area
};

#endif // MAINWINDOW_H
