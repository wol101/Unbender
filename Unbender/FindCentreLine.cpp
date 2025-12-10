#include "FindCentreLine.h"

FindCentreLine::FindCentreLine() {}

// QPoint FindCentreLine::FindNextCentrePoint()
// {
//     QPoint result;

//     std::map<double, QRgb> angleValues;
//     std::vector<QPoint> points = circleEdgePixels(m_image, m_currentPoint.x(), m_currentPoint.y(), m_radius, &angleValues);

//     return result;
// }

// std::vector<QPoint> FindCentreLine::circleEdgePixels(const QImage& image, int cx, int cy, int r, std::map<double, QRgb> *angleValues)
// {
//     std::vector<QPoint> result;

//     int x = r;
//     int y = 0;
//     int decisionOver2 = 1 - x;   // decision criterion

//     while (y <= x)
//     {
//         auto addPoint = [&](int px, int py)
//         {
//             if (px >= 0 && py >= 0 && px < image.width() && py < image.height())
//             {
//                 result.emplace_back(px, py);
//                 if (angleValues)
//                 {
//                     double angle = std::atan2(py, px);
//                     (*angleValues)[angle] = image.pixel(px, py);
//                 }
//             }
//         };

//         // 8-way symmetry
//         addPoint(cx + x, cy + y);
//         addPoint(cx + y, cy + x);
//         addPoint(cx - x, cy + y);
//         addPoint(cx - y, cy + x);
//         addPoint(cx - x, cy - y);
//         addPoint(cx - y, cy - x);
//         addPoint(cx + x, cy - y);
//         addPoint(cx + y, cy - x);

//         y++;
//         if (decisionOver2 <= 0)
//         {
//             decisionOver2 += 2 * y + 1;
//         }
//         else
//         {
//             x--;
//             decisionOver2 += 2 * (y - x) + 1;
//         }
//     }

//     return result;
// }

void FindCentreLine::findEdges()
{
    // get the pixels in a circle around the starting point
    std::vector<Image<uint8_t>::PixelAndLocation> pixelAndLocation = m_image->circleEdgePixels(m_startPoint.x(), m_startPoint.y(), m_radius, true);
}

void FindCentreLine::findTransitions(const std::vector<Image<uint8_t>::PixelAndLocation> &pixelAndLocation)
{
    // are we starting in a background area
    if (pixelAndLocation[0].p < m_threshold)
    {
        for (size_t i = 0; i < pixelAndLocation.size(); i++)
        {

        }
    }
}

void FindCentreLine::setStartPoint(Point newStartPoint)
{
    m_startPoint = newStartPoint;
    m_currentPoint = m_startPoint;
}

void FindCentreLine::setEndPoint(Point newEndPoint)
{
    m_endPoint = newEndPoint;
}

void FindCentreLine::setImage(const Image<uint8_t> *newImage)
{
    m_image = newImage;
}
