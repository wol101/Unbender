#include "FindCentreLine.h"

FindCentreLine::FindCentreLine() {}

void FindCentreLine::straighten()
{
    m_stickList.clear();
    m_thresh = OpenCVTools::thresholdImage(m_img, 100, true);
    std::vector<cv::Point> outline = OpenCVTools::polylineFromBinaryImage(m_thresh);

    std::vector<cv::Point2f> outlinef;
    outlinef.reserve(outline.size());
    for (const auto& p : outline) { outlinef.emplace_back(static_cast<float>(p.x), static_cast<float>(p.y)); }
    float vertexTolerance = 0.001;
    std::vector<cv::Point2f> openOutline = OpenCVTools::splitClosedPolylineRobust(outlinef, m_userPoint1, vertexTolerance);
    OpenCVTools::splitOpenPolylineRobust(openOutline, m_userPoint2, vertexTolerance, m_polyA, m_polyB);
    std::reverse(m_polyB.begin(), m_polyB.end()); // I want both polylines to start at userPoint1

    size_t segments = 100;
    m_centreLine.clear();
    std::vector<cv::Point2f> stick(2);
    for (size_t i = 0; i < segments + 1; ++i)
    {
        float t = float(i) / 100.0f;
        cv::Point2f a = OpenCVTools::pointAtProportion(m_polyA, t);
        cv::Point2f b = OpenCVTools::pointAtProportion(m_polyB, t);
        m_centreLine.push_back(cv::Point2f(0.5f * (a.x + b.x), 0.5f * (a.y + b.y)));

        stick[0] = a; stick[1] = b;
        m_stickList.push_back(stick);
    }

}

void FindCentreLine::straightenMore()
{
    m_stickList.clear();
    bool closed = false;
    bool leftNormals = true;
    std::vector<cv::Point2f> segmentNormals;
    std::vector<cv::Point2f> vertexNormals;
    OpenCVTools::computeNormals(m_centreLine, closed, leftNormals, segmentNormals, vertexNormals);
    std::vector<cv::Point2f> newCentreLine;
    newCentreLine.reserve(m_centreLine.size());
    newCentreLine.push_back(m_centreLine.front());

    std::vector<cv::Point2f> stick(2);
    for (size_t i = 1; i < vertexNormals.size() - 1; ++i)
    {
        cv::Point2f rayOrigin =  m_centreLine[i];
        cv::Point2f rayDir =  vertexNormals[i];
        bool closed = false;
        std::vector<OpenCVTools::RayHit> outHits;
        float eps = std::numeric_limits<float>::epsilon();
        OpenCVTools::intersectRayWithPolylineDeterministic(m_polyA, rayOrigin, rayDir, closed, outHits, eps);
        if (outHits.size() == 0) continue;
        stick[0] = outHits[0].point;
        OpenCVTools::intersectRayWithPolylineDeterministic(m_polyB, rayOrigin, rayDir, closed, outHits, eps);
        if (outHits.size() == 0) continue;
        stick[1] = outHits[0].point;
        newCentreLine.push_back(OpenCVTools::pointAtProportion(stick, 0.5));
        m_stickList.push_back(stick);
    }
    newCentreLine.push_back(m_centreLine.back());

    m_centreLine = newCentreLine;
}

void FindCentreLine::createStraightVersion()
{
    m_straightCentreLine.clear();
    m_straigthStickList.clear();
    m_straightCentreLine.reserve(m_centreLine.size());
    m_straigthStickList.reserve(m_stickList.size());
    m_straightCentreLine.push_back({0.0f, 0.0f});
    for (size_t i = 0; i < m_centreLine.size() - 1; ++i)
    {
        float len = cv::norm(m_centreLine[i + 1] - m_centreLine[i]);
        m_straightCentreLine.push_back({m_straightCentreLine[i].x + len, 0.0f});
    }
    std::vector<cv::Point2f> stick(2);
    for (size_t i = 0; i < m_stickList.size(); ++i)
    {
        float len = cv::norm(m_stickList[i][1] - m_stickList[i][0]);
        stick[0] = {0.0f, -len / 2.0f};
        stick[1] = {0.0f, len / 2.0f};
        m_straigthStickList.push_back(stick);
    }
    m_straightProfile.clear();
    m_straightProfile.push_back({0.0f, 0.0f});
    for (size_t i = 0; i < m_straigthStickList.size(); ++i)
    {
        m_straightProfile.push_back({m_straightCentreLine[i + 1].x, m_straigthStickList[i][1].y});
    }
    m_straightProfile.push_back({m_straightCentreLine[m_straigthStickList.size() + 1].x, 0.0f});
    cv::Point3f axisPoint = {0.0f, 0.0f, 0.0f};
    cv::Point3f axisDir = {1.0f, 0.0f, 0.0f};
    int slices = 64;
    bool capStart = false;
    bool capEnd = false;
    m_straightMesh = OpenCVTools::revolvePolyline(m_straightProfile, axisPoint, axisDir, slices, capStart, capEnd);
}



void FindCentreLine::setImg(const cv::Mat &newImg)
{
    m_img = newImg;
}

void FindCentreLine::setUserPoint1(cv::Point2f newUserPoint1)
{
    m_userPoint1 = newUserPoint1;
}

void FindCentreLine::setUserPoint2(cv::Point2f newUserPoint2)
{
    m_userPoint2 = newUserPoint2;
}

const std::vector<cv::Point2f> &FindCentreLine::polyA() const
{
    return m_polyA;
}

const std::vector<cv::Point2f> &FindCentreLine::polyB() const
{
    return m_polyB;
}

const std::vector<cv::Point2f> &FindCentreLine::centreLine() const
{
    return m_centreLine;
}

const std::vector<std::vector<cv::Point2f> > &FindCentreLine::stickList() const
{
    return m_stickList;
}

const cv::Mat &FindCentreLine::thresh() const
{
    return m_thresh;
}

const std::vector<cv::Point2f> &FindCentreLine::straightCentreLine() const
{
    return m_straightCentreLine;
}

const std::vector<std::vector<cv::Point2f> > &FindCentreLine::straigthStickList() const
{
    return m_straigthStickList;
}

const std::vector<cv::Point2f> &FindCentreLine::straightProfile() const
{
    return m_straightProfile;
}

const OpenCVTools::Mesh &FindCentreLine::straightMesh() const
{
    return m_straightMesh;
}

