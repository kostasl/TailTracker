#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QApplication>
#include <QDebug>
#include <QDateTime>
#include <QObject>

class mainwindow : public QObject
{
public:
    mainwindow(QQmlApplicationEngine& engine);
    void LogEvent(QString txt,int AlertLevel);

    Q_OBJECT
   public slots:
       void cppSlot(const QString &msg) {
           qDebug() << "Called the C++ slot with message:" << msg;
       }


private :
    QObject* txtLog;

};

#endif // MAINWINDOW_H
