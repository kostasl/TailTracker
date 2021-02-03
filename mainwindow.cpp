#include "mainwindow.h"

mainwindow::mainwindow(QQmlApplicationEngine& engine)
{

     QObject* oWindow = engine.rootObjects().first();
     QObject::connect(oWindow  , SIGNAL(qmlSignal(QString)),
                      this, SLOT(cppSlot(QString)));

     // Fetch Point to Window Object
      txtLog = (oWindow->findChild<QObject*>("txtLog")); //QTextObject


}

void mainwindow::LogEvent(QString msg,int AlertLevel)
{

   txtLog->setProperty("text", QDateTime::currentDateTime().toString()+ QString("<b>")+ msg + QString("<br>")); //+ QString("Application Started")

}
