#include "OpenCVTools.h"

#include <QImage>
#include <QPainterPath>

OpenCVTools::OpenCVTools() {}

// routine to convert QImage to OpenCV image
cv::Mat OpenCVTools::convertQImageToMat(const QImage &img)
{
    // Grayscale (QImage::Format_Grayscale8) → CV_8UC1
    if (img.format() == QImage::Format_Grayscale8) {
        return cv::Mat(img.height(), img.width(), CV_8UC1, const_cast<uchar*>(img.bits()), img.bytesPerLine()).clone();
    }

    // RGB (QImage::Format_RGB888) → CV_8UC3 (needs channel swap)
    else if (img.format() == QImage::Format_RGB888)
    {
        cv::Mat mat(img.height(), img.width(), CV_8UC3, const_cast<uchar*>(img.bits()), img.bytesPerLine());
        cv::cvtColor(mat, mat, cv::COLOR_RGB2BGR); // Qt uses RGB, OpenCV expects BGR
        return mat.clone();
    }

    // ARGB (QImage::Format_ARGB32) → CV_8UC4
    else if (img.format() == QImage::Format_ARGB32)
    {
        return cv::Mat(img.height(), img.width(), CV_8UC4, const_cast<uchar*>(img.bits()), img.bytesPerLine()).clone();
    }
    return cv::Mat();
}

// routine to convert OpenCV to QImage image
QImage OpenCVTools::convertMatToQImage(const cv::Mat &mat)
{
    // Grayscale (CV_8UC1) → QImage::Format_Grayscale8
    if (mat.type() == CV_8UC1)
    {
        return QImage(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_Grayscale8).copy();
    }

    // BGR (CV_8UC3) → QImage::Format_RGB888 (with channel swap)
    else if (mat.type() == CV_8UC3)
    {
        QImage img(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_RGB888);
        return img.rgbSwapped();  // OpenCV uses BGR, Qt expects RGB (this routine produces a new image so copy is not needed)
    }

    // BGRA (CV_8UC4) → QImage::Format_ARGB32
    else if (mat.type() == CV_8UC4)
    {
        return QImage(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_ARGB32).copy();
    }
    return QImage();
}

// routine to threshold an image - works with any image type (converts to 8 bit grey if necessary)
cv::Mat OpenCVTools::thresholdImage(const cv::Mat &img, uint8_t grey8Threshold)
{
    cv::Mat thresh, gray8;

    // If the image is not already grayscale, convert it
    // COLOR_BGR2GRAY is weighted luminance formula (ITU‑R BT.601 standard 0.299R + 0.587G + 0.114B)
    if (img.channels() == 3 || img.channels() == 4) { cv::cvtColor(img, gray8, cv::COLOR_BGR2GRAY); }
    else { gray8 = img.clone(); }

    // Ensure it's 8-bit (CV_8U)
    if (gray8.depth() != CV_8U) { gray8.convertTo(gray8, CV_8U); }

    // Ensure it's binary (thresholding if needed)
    cv::threshold(gray8, thresh, grey8Threshold, 255, cv::THRESH_BINARY);

    return gray8;
}


// routine to get an approximate polyline from a binary image
std::vector<cv::Point> OpenCVTools::polylineFromBinaryImage(const cv::Mat &thresh)
{
    std::vector<cv::Point> approx;
    // Find contours
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(thresh, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    for (size_t i = 0; i < contours.size(); ++i) {
        // Approximate contour with polyline
        double epsilon = 0.01 * cv::arcLength(contours[i], true);
        cv::approxPolyDP(contours[i], approx, epsilon, true);
    }

    return approx;
}

// routine to convert an OpenCV polyline to a QPainterPath
QPainterPath OpenCVTools::convertPolylineToQPainterPath(const std::vector<cv::Point> &polyline, bool closed)
{
    QPainterPath path;
    if (polyline.size())
    {
        path.moveTo(polyline.front().x, polyline.front().y);
        for (size_t i = 1; i < polyline.size(); ++i)
        {
            path.lineTo(polyline[i].x, polyline[i].y);
        }
        if (closed)
        {
            if (polyline.back() != polyline.front())
            {
                path.lineTo(polyline.back().x, polyline.back().y);
            }
        }
    }
    return path;
}


