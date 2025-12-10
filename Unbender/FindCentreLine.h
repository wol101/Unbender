#ifndef FINDCENTRELINE_H
#define FINDCENTRELINE_H

#include "Image.h"

class FindCentreLine
{
public:
    FindCentreLine();

    void findEdges();

    void findTransitions(const std::vector<Image<uint8_t>::PixelAndLocation> &pixelAndLocation);

    void setStartPoint(Point newStartPoint);

    void setEndPoint(Point newEndPoint);

    void setImage(const Image<uint8_t> *newImage);

private:
    const Image<uint8_t> *m_image;
    Point m_startPoint;
    Point m_endPoint;
    Point m_currentPoint;
    size_t m_radius = 20;
    uint8_t m_threshold = 1;

};

#endif // FINDCENTRELINE_H
