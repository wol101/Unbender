#ifndef FINDCENTRELINE_H
#define FINDCENTRELINE_H

#include <QPoint>
#include <QImage>

#include <vector>
#include <map>

class FindCentreLine
{
public:
    FindCentreLine();

    QPoint FindNextCentrePoint();

    static std::vector<QPoint> circleEdgePixels(const QImage& image, int cx, int cy, int r, std::map<double, QRgb> *angleValues);

    void setStartPoint(QPoint newStartPoint);

    void setEndPoint(QPoint newEndPoint);

    void setImage(const QImage &newImage);

private:
    QImage m_image;
    QPoint m_startPoint;
    QPoint m_endPoint;
    QPoint m_currentPoint;
    int m_radius = 20;
    int m_delta = 5;

};

#endif // FINDCENTRELINE_H
