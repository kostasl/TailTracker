#include "trackerimageprovider.h"


void trackerImageProvider::setNextFrame(QPixmap frm)
{

    pixmap = frm.copy(0,0,frm.width(),frm.height());// const //cv::Mat::zeros(200,200,CV_8U ); //frm.clone();
}

void trackerImageProvider::setNextFrame(cv::Mat frm)
{

    if (!frm.empty())
        pixmap = QPixmap::fromImage(QImage((unsigned char*) frm.data, frm.cols, frm.rows,
                                                         QImage::Format_RGB888));
}


//Id Could Be Frame Number If We Add Frames to An Array
QPixmap trackerImageProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
      int width = 200;
      int height = 200;

    //QPixmap pixmap;
    //qDebug() << "Image with id" << id << "is requested";

    if (!pixmap.size().width() > 100)
    {
     //QPixmap  pixmap = QPixmap::fromImage(QImage((unsigned char*) currentFrame.data, currentFrame.cols, currentFrame.rows,
     //                                         QImage::Format_RGB888));
       //make temp
        QPixmap pxMapempty(requestedSize.width() > 0 ? requestedSize.width() : width,
                       requestedSize.height() > 0 ? requestedSize.height() : height);
        pxMapempty.fill(QColor("blue").rgba());
        pixmap = pxMapempty;
    }else
   // {

   if (size)
      *size = QSize(pixmap.width(), pixmap.height());

   //pixmap(requestedSize.width() > 0 ? requestedSize.width() : width,
   //               requestedSize.height() > 0 ? requestedSize.height() : height);
   //pixmap.fill(QColor("blue").rgba());
    //}

 return pixmap;
}
