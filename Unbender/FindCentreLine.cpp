#include "FindCentreLine.h"

FindCentreLine::FindCentreLine() {}

QPoint FindCentreLine::FindNextCentrePoint()
{
    QPoint result;

    std::map<double, QRgb> angleValues;
    std::vector<QPoint> points = circleEdgePixels(m_image, m_currentPoint.x(), m_currentPoint.y(), m_radius, &angleValues);

    return result;
}

std::vector<QPoint> FindCentreLine::circleEdgePixels(const QImage& image, int cx, int cy, int r, std::map<double, QRgb> *angleValues)
{
    std::vector<QPoint> result;

    int x = r;
    int y = 0;
    int decisionOver2 = 1 - x;   // decision criterion

    while (y <= x)
    {
        auto addPoint = [&](int px, int py)
        {
            if (px >= 0 && py >= 0 && px < image.width() && py < image.height())
            {
                result.emplace_back(px, py);
                if (angleValues)
                {
                    double angle = std::atan2(py, px);
                    (*angleValues)[angle] = image.pixel(px, py);
                }
            }
        };

        // 8-way symmetry
        addPoint(cx + x, cy + y);
        addPoint(cx + y, cy + x);
        addPoint(cx - x, cy + y);
        addPoint(cx - y, cy + x);
        addPoint(cx - x, cy - y);
        addPoint(cx - y, cy - x);
        addPoint(cx + x, cy - y);
        addPoint(cx + y, cy - x);

        y++;
        if (decisionOver2 <= 0)
        {
            decisionOver2 += 2 * y + 1;
        }
        else
        {
            x--;
            decisionOver2 += 2 * (y - x) + 1;
        }
    }

    return result;
}

void FindCentreLine::setStartPoint(QPoint newStartPoint)
{
    m_startPoint = newStartPoint;
    m_currentPoint = m_startPoint;
}

void FindCentreLine::setEndPoint(QPoint newEndPoint)
{
    m_endPoint = newEndPoint;
}

void FindCentreLine::setImage(const QImage &newImage)
{
    m_image = newImage;
}
