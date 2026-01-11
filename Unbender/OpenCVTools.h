#ifndef OPENCVTOOLS_H
#define OPENCVTOOLS_H

#include <opencv2/opencv.hpp>

#include <QImage>

#include <tuple>

class OpenCVTools
{
public:
    OpenCVTools();

    static cv::Mat convertQImageToMat(const QImage &img);

    static QImage convertMatToQImage(const cv::Mat &mat);

    static cv::Mat convertToGrey(const cv::Mat &img);

    static cv::Mat thresholdImage(const cv::Mat &image, uint8_t grey8Threshold, bool invert);

    static std::vector<cv::Point> polylineFromBinaryImage(const cv::Mat &thresh);

    static QPainterPath convertPolylineToQPainterPath(const std::vector<cv::Point> &polyline, bool joinEnds);
    static QPainterPath convertPolylineToQPainterPath(const std::vector<cv::Point2f> &polyline, bool joinEnds);

    static cv::Mat bgraToBgrOnWhite(const cv::Mat& imgBGRA);

    static cv::Mat bgraToBgrOnBlack(const cv::Mat& imgBGRA);

    static std::string matInfoToString(const cv::Mat& img, const std::string& name = "cv::Mat");

    static std::string qImageInfoToString(const QImage& img, const std::string& name = "QImage");

    static float dist2(const cv::Point2f& a, const cv::Point2f& b);

    static cv::Point2f closestPointOnSegment(const cv::Point2f& A, const cv::Point2f& B, const cv::Point2f& P, float& tOut);

    static std::vector<cv::Point2f> splitClosedPolylineRobust(const std::vector<cv::Point2f>& closedPoly, const cv::Point2f& userPoint, float vertexTolerance);

    static void splitOpenPolylineRobust(const std::vector<cv::Point2f>& poly, const cv::Point2f& userPoint, float vertexTolerance, std::vector<cv::Point2f>& polyA, std::vector<cv::Point2f>& polyB);

};

#endif // OPENCVTOOLS_H
