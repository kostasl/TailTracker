#include "mainwindow.h"

mainwindow::mainwindow(QQmlApplicationEngine& engine,trackerState* trackerstate)
{
      // Get Form Object Pointers And Connect Signaling slots
      QObject* oWindow = engine.rootObjects().first();

     QObject::connect(oWindow  , SIGNAL(qmlSignal(QString)),
                      this, SLOT(cppSlot(QString)));

     oMouseArea = oWindow->findChild<QObject*>(QString("imgMouseArea"));
     QObject::connect(oMouseArea   , SIGNAL(qmlMouseClickSig()),
                      this, SLOT(mouseClickSlot()));

     QObject::connect(oMouseArea  , SIGNAL(qmlMouseReleased()),
                      this, SLOT(OnClickReleasedSlot()));

     QObject::connect(oMouseArea  , SIGNAL(qmlMouseDragSig()),
                      this, SLOT(mouseDragSlot()));

     this->installEventFilter(this); //To KeyPresses
     oWindow->installEventFilter(this);


     // Fetch Point to Window Object
     txtLog = (oWindow->findChild<QObject*>("txtLog")); //QTextObject
     imgScene = (oWindow->findChild<QObject*>("imgTracker")); //Image Item /Connected to custom ImageProvider
     //Pointer to The QImage Type - where we draw the tracker images
     ptrackerView = (trackerImageProvider*)engine.imageProvider("trackerframe");

     ptrackerState = trackerstate; //Save pointer to Tracker State Object
}

void mainwindow::LogEvent(QString msg,int AlertLevel)
{

  QString strMsg = QQmlProperty::read(txtLog, "text").toString();
  strMsg = strMsg.append(  QDateTime::currentDateTime().toString()+ QString("<b>")+ msg + QString("</b><br>") );
   //Append to text Widget
   txtLog->setProperty("text", strMsg ); //+ QString("Application Started")



}

void mainwindow::showCVImage(cv::Mat& img,uint nFrame )
{
    //Update The provider
    //cv::imshow("dgb1",img);

    //QPixmap pixmap = QPixmap::fromImage(QImage((unsigned char*) img.data, img.cols, img.rows,
    //                                         QImage::Format_RGB888));
    assert(ptrackerView);
    //ptrackerView->currentFrame =  img.clone();
    ptrackerView->setNextFrame(img);
    imgScene->setProperty("source",QVariant(QString("image://")+ QLatin1String("trackerframe") + QString("/") + QString::number(nFrame) ) );

    /// Draw Overlay Info From This Window //
    //if (ptDrag)
    //    cv::circle(frameScene,*ptDrag,5,cv::Scalar(200,200,0),2);


}

bool mainwindow::eventFilter(QObject *obj, QEvent *event) {

    //qDebug() << "Event:" << event->type();
      if (event->type() == QEvent::KeyPress) {
         QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
         QString strkey = keyEvent->text();
         qDebug() << "KeyPress:" << strkey;

         ptrackerState->processInputKey(keyEvent->key());

      }

}


mainwindow::~mainwindow()
{
    //delete ptrackerState;
}
