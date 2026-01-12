#ifndef FINDCENTRELINE_H
#define FINDCENTRELINE_H

#include "OpenCVTools.h"

class FindCentreLine
{
public:
    FindCentreLine();

    void straighten();

    void straightenMore();

    void createStraightVersion();

    void setImg(const cv::Mat &newImg);

    void setUserPoint1(cv::Point2f newUserPoint1);

    void setUserPoint2(cv::Point2f newUserPoint2);

    const std::vector<cv::Point2f> &polyA() const;

    const std::vector<cv::Point2f> &polyB() const;

    const std::vector<cv::Point2f> &centreLine() const;

    const std::vector<std::vector<cv::Point2f> > &stickList() const;

    const cv::Mat &thresh() const;

    const std::vector<cv::Point2f> &straightCentreLine() const;

    const std::vector<std::vector<cv::Point2f> > &straigthStickList() const;

    const std::vector<cv::Point2f> &straightProfile() const;

    const OpenCVTools::Mesh &straightMesh() const;

private:
    std::vector<cv::Point2f> m_polyA;
    std::vector<cv::Point2f> m_polyB;
    std::vector<cv::Point2f> m_centreLine;

    cv::Mat m_img;
    cv::Mat m_thresh;

    cv::Point2f m_userPoint1;
    cv::Point2f m_userPoint2;

    std::vector<std::vector<cv::Point2f> > m_stickList;

    std::vector<cv::Point2f> m_straightCentreLine;
    std::vector<std::vector<cv::Point2f> > m_straigthStickList;
    std::vector<cv::Point2f> m_straightProfile;
    OpenCVTools::Mesh m_straightMesh;
};

#endif // FINDCENTRELINE_H
