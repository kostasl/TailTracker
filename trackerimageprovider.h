#ifndef TRACKERIMAGEPROVIDER_H
#define TRACKERIMAGEPROVIDER_H

#include <QQuickImageProvider>
#include <QImage>
#include <QPixmap>
#include <QDebug>

////Open CV
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/core/core.hpp>
#include "opencv2/imgproc/imgproc.hpp"


class trackerImageProvider : public QQuickImageProvider
{
public:
    trackerImageProvider(): QQuickImageProvider(QQuickImageProvider::Pixmap)
    {
         //currentFrame = cv::Mat::zeros(200,200,CV_8U );
    }

    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize) override;

    //Hold local copy of next opencv Tracker Frame
    void setNextFrame(QPixmap frm);
    void setNextFrame(cv::Mat frm);

    cv::Mat currentFrame;

private:

   QPixmap pixmap;


};

#endif // TRACKERIMAGEPROVIDER_H
