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
#include <trackerstate.h>

class trackerState;

class mainwindow : public QObject
{
public:
    mainwindow(QQmlApplicationEngine& engine,trackerState* trackerState);
    ~mainwindow();
    void LogEvent(QString txt,int AlertLevel);
    void showCVImage(cv::Mat& img,uint nFrame );
    Q_OBJECT

   public slots:
       void cppSlot(const QString &msg) {
           qDebug() << "Called the cpp C++ slot with message:" << msg;
       }
       void inputFileChangedSlot(const QString &fileName)
       {
           qDebug() << "Change input to :" << fileName;
           QUrl inF(fileName); //Convert Url To AbsPath
           ptrackerState->addinputFile(inF.toLocalFile());
       }
       void outputFileChangedSlot(const QString &fileName)
       {
            qDebug() << "Output file set to to :" << fileName;
             QUrl inF(fileName); //Convert Url To AbsPath
            ptrackerState->setOutputFile(inF.toLocalFile());

       }
       void startTrackingSlot()
       {
           ptrackerState->startTracking();
       }
       //Mouse Is PressedAndHold
       void mouseClickSlot() {
           mouseX = oMouseArea->property("mouseX").toInt();
           mouseY = oMouseArea->property("mouseY").toInt();

           //qDebug() << "Mouse Click slot coords:" << mouseX << "," << mouseY;
           //Click Triggered on Button Release
           // 1st Set Top Of Tail On Point and Display Arrow
           // 2nd point Arrow for tail direction
           if (ptrackerState->FitTailConfigState == 0)
           {
                ptrackerState->ptTailRoot.x = mouseX;
                ptrackerState->ptTailRoot.y = mouseY;
                ptrackerState->tailsplinefit[1].x = mouseX+2;
                ptrackerState->tailsplinefit[1].y = mouseY+2;
                ptrackerState->FitTailConfigState = 1; //Mouse Released so stop Dragging
           }

       }

       void mouseDragSlot() {
           mouseX = oMouseArea->property("mouseX").toInt();
           mouseY = oMouseArea->property("mouseY").toInt();
               //qDebug() << "Mouse move slot coords:" << mouseX << "," << mouseY;

               //Continue to Set 2nd Point While Mouse Dragging
               if (ptrackerState->FitTailConfigState == 1)
               {
                    ptrackerState->tailsplinefit[1].x = mouseX;
                    ptrackerState->tailsplinefit[1].y = mouseY;
               }


       }
       void OnClickReleasedSlot()
       {
           mouseX = oMouseArea->property("mouseX").toInt();
           mouseY = oMouseArea->property("mouseY").toInt();
            //qDebug() << "Mouse Released slot coords:" << mouseX << "," << mouseY;
           //User Released Button - Reset Config State
            if (ptrackerState->FitTailConfigState == 1)
            {
                ptrackerState->tailsplinefit[1].x = mouseX;
                ptrackerState->tailsplinefit[1].y = mouseY;
                ptrackerState->FitTailConfigState = 2; //State to reset/recalc The spine - Read in processVideo
            }

       }


public:
       /// Activate Busy Indicator
       void setBusyOn();
       void setBusyOff();

cv::Mat frameScene; //CvMat Last Frame Drawn
QImage qimg; //SCene Image Updated in showCVImage Image
trackerImageProvider* ptrackerView;
int mouseX,mouseY; //mouse Position within The TrackerImage Area


protected:
    bool eventFilter(QObject *obj, QEvent *event);

private :
    QObject* txtLog;
    QObject* imgScene;
    QObject* oMouseArea; //Video frame Mouse Are
    QObject* busyIndicator;
    QObject* oWindow;
    QObject* oInputFileDialog;
    QObject* oOutputFileDialog;
    QObject*  oButtonTrack;
    trackerState*  ptrackerState; //POinter to External instance of tracker state - Passed from Constructor
};

#endif // MAINWINDOW_H
