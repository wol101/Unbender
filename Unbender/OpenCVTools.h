#ifndef OPENCVTOOLS_H
#define OPENCVTOOLS_H

#include <opencv2/opencv.hpp>

#include <QImage>

#include <string>


class OpenCVTools
{
public:
    OpenCVTools();

    struct RayHit
    {
        cv::Point2f point;     // intersection point
        float t;               // ray parameter (signed)
        float u;               // segment parameter [0,1]
        size_t segmentIndex;   // which segment was hit
    };

    struct Mesh
    {
        std::vector<cv::Point3f> vertices;
        std::vector<cv::Point3f> normals;
        std::vector<cv::Point2f> uvs;
        std::vector<cv::Vec3i>   triangles;
    };

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

    static cv::Point2f pointAtProportion(const std::vector<cv::Point2f>& poly, float t);

    static std::vector<cv::Point2f> intersectPolylines(const std::vector<cv::Point2f>& poly1, const std::vector<cv::Point2f>& poly2);

    static bool segmentIntersection(const cv::Point2f& A, const cv::Point2f& B, const cv::Point2f& C, const cv::Point2f& D, cv::Point2f& out);

    static cv::Point2f normalize(const cv::Point2f& v);
    static cv::Point3f normalize(const cv::Point3f& v);

    static void computeNormals(const std::vector<cv::Point2f>& polyline, bool closed, bool leftNormals, std::vector<cv::Point2f>& segmentNormals, std::vector<cv::Point2f>& vertexNormals);

    static void intersectRayWithPolylineDeterministic(const std::vector<cv::Point2f>& polyline, const cv::Point2f& rayOrigin, const cv::Point2f& rayDir, bool closed, std::vector<OpenCVTools::RayHit>& outHits, float eps = 1e-6);

    static cv::Point3f rotateAroundAxis(const cv::Point3f& p, const cv::Point3f& axisPoint, const cv::Point3f& axisDirNorm, float angle);

    static OpenCVTools::Mesh revolvePolyline(const std::vector<cv::Point2f>& polyline2D, const cv::Point3f& axisPoint, const cv::Point3f& axisDir, int slices, bool capStart, bool capEnd);

    static std::string meshToOBJ(const OpenCVTools::Mesh& mesh, const std::string& objectName = "mesh");

};

#endif // OPENCVTOOLS_H
