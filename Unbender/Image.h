#ifndef IMAGE_H
#define IMAGE_H

#include <cmath>
#include <vector>
#include <stdexcept>
#include <cassert>
#include <map>
#include <numbers>

class Point {
public:
    Point(std::size_t x = 0, std::size_t y = 0) : m_x(x), m_y(y) {}

    // Accessors
    std::size_t x() const noexcept { return m_x; }
    std::size_t y() const noexcept { return m_y; }

    void setX(std::size_t x) noexcept { m_x = x; }
    void setY(std::size_t y) noexcept { m_y = y; }

    // Comparison operator for std::map key ordering
    bool operator<(const Point& other) const noexcept
    {
        if (m_x < other.m_x) return true;
        if (m_x > other.m_x) return false;
        return m_y < other.m_y;
    }

    // Equality operator (optional, useful for comparisons)
    bool operator==(const Point& other) const noexcept
    {
        return m_x == other.m_x && m_y == other.m_y;
    }

private:
    std::size_t m_x;
    std::size_t m_y;
};


template <typename PixelType>
class Image {
public:
    struct PixelAndLocation { PixelType p; std::size_t x; std::size_t y; };

    Image(std::size_t width, std::size_t height, const PixelType& init = PixelType())
        : m_width(width), m_height(height), m_data(width * height, init) {}

    std::size_t width() const noexcept { return m_width; }
    std::size_t height() const noexcept { return m_height; }

    // Pixel accessor with bounds checking in debug mode
    PixelType& operator()(std::size_t x, std::size_t y)
    {
#ifndef NDEBUG
        if (x >= m_width || y >= m_height) { throw std::out_of_range("Image pixel access out of range"); }
#endif
        return m_data[y * m_width + x];
    }

    const PixelType& operator()(std::size_t x, std::size_t y) const
    {
#ifndef NDEBUG
        if (x >= m_width || y >= m_height) { throw std::out_of_range("Image pixel access out of range"); }
#endif
        return m_data[y * m_width + x];
    }

    void setRow(const PixelType *data, std::size_t y)
    {
#ifndef NDEBUG
        if (y >= m_height) { throw std::out_of_range("Image row access out of range"); }
#endif
        std::copy_n(data, m_width, &m_data[y * m_width]);
    }

    const PixelType* row(std::size_t y)
    {
#ifndef NDEBUG
        if (y >= m_height) { throw std::out_of_range("Image row access out of range"); }
#endif
        return &m_data[y * m_width];
    }

    const std::vector<PixelType> &data() const { return m_data; }

    // Use Bresenham’s line algorithm to get the pixel values along a straight line
    std::vector<PixelAndLocation> linePixels(std::size_t x0, std::size_t y0, std::size_t x1, std::size_t y1) const {
#ifndef NDEBUG
        if (x0 >= m_width || y0 >= m_height || x1 >= m_width || y1 >= m_height) {
            throw std::out_of_range("Line endpoints out of range");
        }
#endif
        std::vector<PixelAndLocation> result;

        int64_t dx = std::abs((int)x1 - (int)x0);
        int64_t dy = -std::abs((int)y1 - (int)y0);
        int64_t sx = (x0 < x1) ? 1 : -1;
        int64_t sy = (y0 < y1) ? 1 : -1;
        int64_t err = dx + dy;

        std::size_t x = x0;
        std::size_t y = y0;

        while (true)
        {
            result.push_back(PixelAndLocation{(*this)(x, y), x, y});
            if (x == x1 && y == y1) break;
            int e2 = 2 * err;
            if (e2 >= dy) { err += dy; x += sx; }
            if (e2 <= dx) { err += dx; y += sy; }
        }

        return result;
    }

    // Use Midpoint Circle Algorithm (Discrete Raster Circle) to get the points
    std::vector<PixelAndLocation> circleEdgePixels(size_t cx, size_t cy, size_t r, bool sortByAngle = false)
    {
#ifndef NDEBUG
        if (cx + r >= m_width || cy + r >= m_height || cx < r || cy < r) {
            throw std::out_of_range("Circle edges out of range");
        }
#endif
        std::vector<PixelAndLocation> result;
        int64_t x = r;
        int64_t y = 0;
        int64_t decisionOver2 = 1 - x;   // decision criterion

        while (y <= x)
        {
            auto addPoint = [&](int px, int py)
            {
                result.push_back(PixelAndLocation{(*this)(px, py), px, py});
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
            if (decisionOver2 <= 0) { decisionOver2 += 2 * y + 1; }
            else { x--; decisionOver2 += 2 * (y - x) + 1; }
        }
        if (sortByAngle)
        {
            std::map<double, PixelAndLocation> angleMap;
            for (auto &&it : result)
            {
                double angle = std::atan2(it->y(), it->x()) + 2 * std::numbers::pi;
                angleMap[angle] = *it;
            }
            result.clear();
            for (auto &&it : angleMap)
            {
                result.push_back(*it);
           }
        }
        return result;
    };

private:
    std::size_t m_width;
    std::size_t m_height;
    std::vector<PixelType> m_data;
};

#endif // IMAGE_H
