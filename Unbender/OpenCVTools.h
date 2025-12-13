#ifndef OPENCVTOOLS_H
#define OPENCVTOOLS_H

#include <opencv2/opencv.hpp>

#include <QImage>

class OpenCVTools
{
public:
    OpenCVTools();

    static cv::Mat convertQImageToMat(const QImage &img);

    static cv::Mat thresholdImage(const cv::Mat &image, uint8_t grey8Threshold);

    static std::vector<cv::Point> polylineFromBinaryImage(const cv::Mat &thresh);

    static QPainterPath convertPolylineToQPainterPath(const std::vector<cv::Point> &polyline, bool closed);


};

#endif // OPENCVTOOLS_H
