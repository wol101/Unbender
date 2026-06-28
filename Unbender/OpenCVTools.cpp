#include "OpenCVTools.h"

#include <QImage>
#include <QPainterPath>
#include <QDebug>

#include <sstream>
#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

OpenCVTools::OpenCVTools() {}

// routine to convert QImage to OpenCV image
// this assumes that the QImage is already in one of endian agnostic formats
cv::Mat OpenCVTools::convertQImageToMat(const QImage &img)
{
    while (true)
    {
        // Grayscale (QImage::Format_Grayscale8) → CV_8UC1
        if (img.format() == QImage::Format_Grayscale8)
        {
            return cv::Mat(img.height(), img.width(), CV_8UC1, const_cast<uchar*>(img.bits()), img.bytesPerLine()).clone(); // clone required so the returned image owns data
        }

        // RGB (QImage::Format_RGB888) → CV_8UC3 (needs channel swap)
        if (img.format() == QImage::Format_RGB888)
        {
            cv::Mat mat(img.height(), img.width(), CV_8UC3, const_cast<uchar*>(img.bits()), img.bytesPerLine()); // note mat does not own data
            cv::Mat matBGR;
            cv::cvtColor(mat, matBGR, cv::COLOR_RGB2BGR); // Qt uses RGB, OpenCV expects BGR
            return matBGR;
        }

        // RGB (QImage::Format_RGBA8888) → CV_8UC4 (needs channel swap and throws away the unused alpha channel)
        if (img.format() == QImage::Format_RGBA8888)
        {
            cv::Mat mat(img.height(), img.width(), CV_8UC4, const_cast<uchar*>(img.bits()), img.bytesPerLine()); // note mat does not own data
            cv::Mat matBGRA;
            cv::cvtColor(mat, matBGRA, cv::COLOR_RGBA2BGRA); // Qt uses ARGB, OpenCV expects BGRA
            return matBGRA;
        }
        std::string s = qImageInfoToString(img);
        qDebug() << "OpenCVTools::convertQImageToMat error: unable to convert image\n" << s.c_str();
        break;
    }
    return cv::Mat();
}

// routine to convert OpenCV to QImage image
QImage OpenCVTools::convertMatToQImage(const cv::Mat &mat)
{
    while (true)
    {
        // Grayscale (CV_8UC1) → QImage::Format_Grayscale8
        if (mat.type() == CV_8UC1)
        {
            return QImage(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_Grayscale8).copy();
        }

        // BGR (CV_8UC3) → QImage::Format_RGB888 (with channel swap)
        if (mat.type() == CV_8UC3)
        {
            cv::Mat matRGB;
            cv::cvtColor(mat, matRGB, cv::COLOR_BGR2RGB);
            return QImage(matRGB.data, matRGB.cols, matRGB.rows, matRGB.step, QImage::Format_RGB888).copy();
        }

        // BGRA (CV_8UC4) → QImage::Format_RGBA8888
        else if (mat.type() == CV_8UC4)
        {
            cv::Mat matRGBA;
            cv::cvtColor(mat, matRGBA, cv::COLOR_BGRA2RGBA);
            return QImage(matRGBA.data, matRGBA.cols, matRGBA.rows, matRGBA.step, QImage::Format_RGBA8888).copy();
        }
        std::string s = matInfoToString(mat);
        qDebug() << "OpenCVTools::convertMatToQImage error: unable to convert image\n" << s.c_str();
        break;
    }
    return QImage();
}

// routine to convert most formats to greyscale
cv::Mat OpenCVTools::convertToGrey(const cv::Mat &img)
{
    cv::Mat grey;
    // Handle common cases
    while (true)
    {
        if (img.channels() == 3)
        {
            // BGR → Gray
            cv::cvtColor(img, grey, cv::COLOR_BGR2GRAY); // COLOR_BGR2GRAY is weighted luminance formula (ITU‑R BT.601 standard 0.299R + 0.587G + 0.114B)
            break;
        }
        if (img.channels() == 4)
        {
            // using cv::cvtColor(bgr, grey, cv::COLOR_BGRA2GRAY) or cv::cvtColor(img, bgr, cv::COLOR_BGRA2BGR) does not work very well
            // because it just throws away the alpha channel and that may be masking unwanted colours
            // it is better to blend the image as if it was being drawn on a background
            cv::Mat bgr = bgraToBgrOnWhite(img);
            cv::cvtColor(bgr, grey, cv::COLOR_BGR2GRAY);
            break;
        }
        if (img.channels() == 1)
        {
            // Already grayscale
            grey = img.clone();
            break;
        }
        std::string s = matInfoToString(img);
        qDebug() << "OpenCVTools::convertToGrey error: unable to convert image to greyscale\n" << s.c_str();
        return grey;
    }

    // Check it's 8-bit (CV_8U)
    if (grey.depth() != CV_8U)
    {
        std::string s = matInfoToString(img);
        qDebug() << "OpenCVTools::convertToGrey error: greyscale image not 8 bit\n" << s.c_str();
        return cv::Mat();
    }
    return grey;
}

// routine to threshold an image - works with any image type (converts to 8 bit grey if necessary)
cv::Mat OpenCVTools::thresholdImage(const cv::Mat &img, uint8_t grey8Threshold, bool invert)
{
    cv::Mat thresh;
    // std::string s = matInfoToString(img, "input"); qDebug() << s.c_str();

    cv::Mat grey = convertToGrey(img);
    if (grey.empty())
    {
        qDebug() << "OpenCVTools::thresholdImage error: unable to convert image to greyscale\n";
        return thresh;
    }

    if (invert) { cv::threshold(grey, thresh, grey8Threshold, 255, cv::THRESH_BINARY_INV); }
    else { cv::threshold(grey, thresh, grey8Threshold, 255, cv::THRESH_BINARY); }
    // s = matInfoToString(thresh, "after threshold"); qDebug() << s.c_str();

    return thresh;
}

// routine to get an approximate polyline from a binary image
std::vector<cv::Point> OpenCVTools::polylineFromBinaryImage(const cv::Mat &thresh)
{
    std::vector<cv::Point> result;
    // Find contours
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(thresh, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    // find the longest contour
    auto it = std::max_element(contours.begin(), contours.end(), [](const auto& a, const auto& b) { return a.size() < b.size(); });
    if (it == contours.end()) { return result; } // no contours found

    // Approximate contour with polyline
    double epsilon = 0.0001 * cv::arcLength(*it, true);
    cv::approxPolyDP(*it, result, epsilon, true);

    return result;
}

// routine to convert an OpenCV polyline to a QPainterPath
// jointEnds allows a OpenCV closed line to be drawn properly using the path
QPainterPath OpenCVTools::convertPolylineToQPainterPath(const std::vector<cv::Point> &polyline, bool joinEnds)
{
    QPainterPath path;
    if (polyline.size())
    {
        path.moveTo(polyline.front().x, polyline.front().y);
        for (size_t i = 1; i < polyline.size(); ++i)
        {
            path.lineTo(polyline[i].x, polyline[i].y);
        }
        if (joinEnds)
        {
            if (polyline.back() != polyline.front())
            {
                path.lineTo(polyline.front().x, polyline.front().y);
            }
        }
    }
    return path;
}

QPainterPath OpenCVTools::convertPolylineToQPainterPath(const std::vector<cv::Point2f> &polyline, bool joinEnds)
{
    std::vector<cv::Point> polyline2;
    polyline2.reserve(polyline.size());
    for (const auto& p : polyline) { polyline2.emplace_back(static_cast<int>(p.x), static_cast<int>(p.y)); }
    return convertPolylineToQPainterPath(polyline2, joinEnds);
}

std::vector<cv::Point2f> outlinef;

// For each pixel (B, G, R, A) in BGRA:
//         A
// B' = ------- * B + (1 - A/255) * 255
//        255
//         A
// G' = ------- * G + (1 - A/255) * 255
//        255
//         A
// R' = ------- * R + (1 - A/255) * 255
//        255
// This blends the pixel with white according to alpha.

cv::Mat OpenCVTools::bgraToBgrOnWhite(const cv::Mat& imgBGRA)
{
    CV_Assert(imgBGRA.type() == CV_8UC4);

    cv::Mat bgr(imgBGRA.rows, imgBGRA.cols, CV_8UC3);

    for (int y = 0; y < imgBGRA.rows; ++y) {
        const cv::Vec4b* srcRow = imgBGRA.ptr<cv::Vec4b>(y);
        cv::Vec3b* dstRow = bgr.ptr<cv::Vec3b>(y);

        for (int x = 0; x < imgBGRA.cols; ++x) {
            uchar a = srcRow[x][3];
            float alpha = a / 255.0f;

            dstRow[x][0] = cv::saturate_cast<uchar>(srcRow[x][0] * alpha + 255 * (1 - alpha)); // B
            dstRow[x][1] = cv::saturate_cast<uchar>(srcRow[x][1] * alpha + 255 * (1 - alpha)); // G
            dstRow[x][2] = cv::saturate_cast<uchar>(srcRow[x][2] * alpha + 255 * (1 - alpha)); // R
        }
    }

    return bgr;
}

// Convert BGRA image to BGR composited on a black background
cv::Mat OpenCVTools::bgraToBgrOnBlack(const cv::Mat& imgBGRA)
{
    CV_Assert(imgBGRA.type() == CV_8UC4);

    cv::Mat bgr(imgBGRA.rows, imgBGRA.cols, CV_8UC3);

    for (int y = 0; y < imgBGRA.rows; ++y) {
        const cv::Vec4b* srcRow = imgBGRA.ptr<cv::Vec4b>(y);
        cv::Vec3b* dstRow = bgr.ptr<cv::Vec3b>(y);

        for (int x = 0; x < imgBGRA.cols; ++x) {
            uchar a = srcRow[x][3];
            float alpha = a / 255.0f;

            dstRow[x][0] = cv::saturate_cast<uchar>(srcRow[x][0] * alpha); // B
            dstRow[x][1] = cv::saturate_cast<uchar>(srcRow[x][1] * alpha); // G
            dstRow[x][2] = cv::saturate_cast<uchar>(srcRow[x][2] * alpha); // R
        }
    }

    return bgr;
}


std::string OpenCVTools::matInfoToString(const cv::Mat& img, const std::string& name)
{
    std::stringstream ss;
    ss << name << " info:\n";
    ss << " - Size: " << img.cols << " x " << img.rows << "\n";
    ss << " - Channels: " << img.channels() << "\n";
    ss << " - Depth: " << img.depth() << "\n";
    ss << " - Type: " << img.type() << " (" << cv::typeToString(img.type()) << ")\n";
    ss << " - Empty: " << (img.empty() ? "yes" : "no") << "\n";
    return ss.str();
}

std::string OpenCVTools::qImageInfoToString(const QImage& img, const std::string& name)
{
    std::stringstream ss;
    ss << name << " info:\n";
    ss << " - Size: " << img.width() << " x " << img.height() << "\n";
    ss << " - Depth: " << img.depth() << " bits per pixel\n";
    ss << " - Format: " << img.format() << "\n";   // enum value
    ss << " - Bytes per line: " << img.bytesPerLine() << "\n";
    ss << " - Color count: " << img.colorCount() << "\n";
    ss << " - Is null: " << (img.isNull() ? "yes" : "no") << "\n";
    return ss.str();
}

float OpenCVTools::dist2(const cv::Point2f& a, const cv::Point2f& b)
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return dx*dx + dy*dy;
}

// Returns the closest point on segment AB to point P.
// Also returns the parametric t in [0,1].
cv::Point2f OpenCVTools::closestPointOnSegment(const cv::Point2f& A, const cv::Point2f& B, const cv::Point2f& P, float& tOut)
{
    cv::Point2f AB = B - A;
    float ab2 = AB.dot(AB);

    if (ab2 == 0.0) {
        tOut = 0.0;
        return A; // Degenerate segment
    }

    float t = (P - A).dot(AB) / ab2;
    t = std::max(0.0f, std::min(1.0f, t));
    tOut = t;
    return A + AB * t;
}

// Split a closed polyline based on a user point:
//
// If user point is close to a vertex:
// •    Within vertexTolerance → split at that vertex
// •    No new point inserted
// •    Polyline starts at that vertex
// Otherwise:
// •    Compute closest point on the closest segment
// •    Insert that point
// •    Rotate so it becomes the start
// •    Polyline becomes open
//
// This is a version that is robust for:
// •   Self intersecting contours (figure 8, bow ties, spirals, etc.)
// •   Contours with repeated vertices
// •   Contours where the closest point lies on an edge that crosses itself
// •   Contours where the user point is near a vertex but not necessarily the “first” instance of that vertex
// The key idea is:
// 1.  Treat the polyline as a sequence of segments, not a topological loop.
// 2.  Find the closest point on any segment or vertex.
// 3.  If the closest vertex is within tolerance, split at that vertex index.
// 4.  Otherwise, split at the closest point on the closest segment, inserting a new point between the correct indices, even if that segment participates in a self intersection.
// 5.  Rotate the sequence so the split point becomes index 0.
// 6.  Do not attempt to “fix” or reorder self intersections — preserve the original order exactly.

std::vector<cv::Point2f> OpenCVTools::splitClosedPolylineRobust(const std::vector<cv::Point2f>& closedPoly, const cv::Point2f& userPoint, float vertexTolerance)
{
    std::vector<cv::Point2f> result;
    const int N = closedPoly.size();
    if (N < 2) return closedPoly;

    // Detect and ignore closing duplicate
    bool hasClosingDup = (closedPoly.front() == closedPoly.back());
    int M = hasClosingDup ? N - 1 : N;

    // Best candidate tracking
    float bestDist2 = std::numeric_limits<float>::max();
    int bestVertex = -1;
    float bestT = 0.0f;
    cv::Point2f bestPoint;

    // --- 1. Check vertices first ---
    for (int i = 0; i < M; ++i)
    {
        float d2 = dist2(closedPoly[i], userPoint);
        if (d2 < bestDist2)
        {
            bestDist2 = d2;
            bestVertex = i;
            bestPoint = closedPoly[i];
        }
    }
    if (bestVertex >= 0 && std::sqrt(bestDist2) <= vertexTolerance) // and if less than the threshold split at point
    {
        // Split at vertex
        result.reserve(M);
        for (int k = 0; k < M; ++k)
        {
            result.push_back(closedPoly[(bestVertex + k) % M]);
        }
        if (result.front() != result.back()) // Add the first point to the end if necessary
        {
            result.push_back(result.front());
        }
        return result;
    }

    bestDist2 = std::numeric_limits<float>::max();
    int bestSeg = -1;
    // --- 2. Check segments ---
    for (int i = 0; i < M; ++i)
    {
        int j = (i + 1) % M;
        float t;
        cv::Point2f cp = closestPointOnSegment(closedPoly[i], closedPoly[j], userPoint, t);
        float d2 = dist2(cp, userPoint);
        if (d2 < bestDist2)
        {
            bestDist2 = d2;
            bestSeg = i;
            bestT = t;
            bestPoint = cp;
        }
    }

    // --- 3. If closest point is a vertex then split and do not create an extra point
    if (bestT < std::numeric_limits<float>::epsilon() || bestT > (1.0f - std::numeric_limits<float>::epsilon()))
    {
        // Split at vertex
        bestVertex = (bestT < std::numeric_limits<float>::epsilon()) ? bestSeg : (bestSeg + 1) % M;
        result.reserve(M);
        for (int k = 0; k < M; ++k)
        {
            result.push_back(closedPoly[(bestVertex + k) % M]);
        }
        if (result.front() != result.back()) // Add the first point to the end if necessary
        {
            result.push_back(result.front());
        }
        return result;
    }

    // --- 4. Split at edge (insert new point) ---

    std::vector<cv::Point2f> temp;
    temp.reserve(M + 1);

    for (int k = 0; k < M; ++k)
    {
        temp.push_back(closedPoly[k]);
        if (k == bestSeg)
        {
            temp.push_back(bestPoint); // Insert split point
        }
    }

    // --- 5. Rotate so split point is first ---
    int splitIdx = bestSeg + 1;
    int newSize = temp.size();
    result.reserve(newSize);
    for (int k = 0; k < newSize; ++k)
    {
        result.push_back(temp[(splitIdx + k) % newSize]);
    }

    // --- 6. Add the first point to the end if necessary
    if (result.front() != result.back())
    {
        result.push_back(result.front());
    }

    return result;
}

// Split an open polyline based on a user point and return the two new polylines:
//
// Since the polyline is open, we no longer “rotate” it. Instead, we cut it into:
// •   polyA: from the start of the original polyline up to the split point
// •   polyB: from the split point to the end of the original polyline
// The split point is chosen using the same criteria as before:
// 1.  Compute the closest point on the polyline (vertices + segments).
// 2.  If the closest vertex is within the user supplied tolerance → split at that vertex.
// 3.  Otherwise → split at the closest point on the closest segment, inserting a new point.
// This version is robust for self intersecting open polylines because it never assumes any topology — it only uses the index order.

void OpenCVTools::splitOpenPolylineRobust(const std::vector<cv::Point2f>& poly, const cv::Point2f& userPoint, float vertexTolerance, std::vector<cv::Point2f>& polyA, std::vector<cv::Point2f>& polyB)
{
    const int N = poly.size();
    polyA.clear();
    polyB.clear();

    if (N < 2)
    {
        polyA = poly;
        return;
    }

    float bestDist2 = std::numeric_limits<float>::max();
    int bestVertex = -1;
    float bestT = 0.0;
    cv::Point2f bestPoint;

    // Check vertices first
    for (int i = 0; i < N; ++i)
    {
        float d2 = dist2(poly[i], userPoint);
        if (d2 < bestDist2)
        {
            bestDist2 = d2;
            bestVertex = i;
            bestPoint = poly[i];
        }
    }
    // Split at vertex
    if (bestVertex >= 0 && std::sqrt(bestDist2) <= vertexTolerance)
    {
        polyA.insert(polyA.end(), poly.begin(), poly.begin() + bestVertex + 1);
        polyB.insert(polyB.end(), poly.begin() + bestVertex, poly.end());
        return;
    }

    // Check segments
    int bestSeg = -1;
    bestDist2 = std::numeric_limits<float>::max();
    for (int i = 0; i < N - 1; ++i)
    {
        float t;
        cv::Point2f cp = closestPointOnSegment(poly[i], poly[i+1], userPoint, t);
        float d2 = dist2(cp, userPoint);
        if (d2 < bestDist2)
        {
            bestDist2 = d2;
            bestSeg = i;
            bestT = t;
            bestPoint = cp;
        }
    }

    // If closest point is a vertex then split and do not create an extra point
    if (bestT < std::numeric_limits<float>::epsilon() || bestT > (1.0f - std::numeric_limits<float>::epsilon()))
    {
        // Split at vertex
        bestVertex = (bestT < std::numeric_limits<float>::epsilon()) ? bestSeg : (bestSeg + 1);
        polyA.insert(polyA.end(), poly.begin(), poly.begin() + bestVertex + 1);
        polyB.insert(polyB.end(), poly.begin() + bestVertex, poly.end());
        return;
    }

    // Split at segment (insert new point)
    bestVertex = bestSeg;
    polyA.insert(polyA.end(), poly.begin(), poly.begin() + bestVertex + 1);
    polyA.push_back(bestPoint);

    polyB.push_back(bestPoint);
    polyB.insert(polyB.end(), poly.begin() + bestVertex + 1, poly.end());
}

// poly — std::vector<cv::Point2f>
// t — proportion in [0,1]
// Returns the interpolated point at distance t * total_length.

cv::Point2f OpenCVTools::pointAtProportion(const std::vector<cv::Point2f>& poly, float t)
{
    const int N = poly.size();
    if (N == 0)
        return cv::Point2f();
    if (N == 1)
        return poly[0];

    // Clamp proportion
    t = std::max(0.0f, std::min(1.0f, t));

    // Compute total length
    float totalLen = 0.0;
    std::vector<float> segLen(N - 1);
    for (int i = 0; i < N - 1; ++i)
    {
        segLen[i] = cv::norm(poly[i+1] - poly[i]);
        totalLen += segLen[i];
    }

    if (totalLen == 0.0)
        return poly[0];

    float target = t * totalLen;

    // Walk segments until we reach the target
    float accum = 0.0;
    for (int i = 0; i < N - 1; ++i)
    {
        if (accum + segLen[i] >= target)
        {
            float localT = (target - accum) / segLen[i];
            return poly[i] + (poly[i+1] - poly[i]) * localT;
        }
        accum += segLen[i];
    }

    // Numerical edge case: return last point
    return poly.back();
}


// ------------------------------------------------------------
// Find all intersection points between two polylines
// ------------------------------------------------------------
// computes all intersection points between two OpenCV polylines. It works for:
// - Open or closed polylines
// - Self-intersecting shapes
// - Horizontal, vertical, and diagonal segments
// - Floating-point precision
// The algorithm is simple and deterministic:
// - Treat each polyline as a sequence of line segments.
// - Test every segment of polyline A against every segment of polyline B.
// - Use a stable segment–segment intersection routine.
// - Collect all intersection points.

std::vector<cv::Point2f> OpenCVTools::intersectPolylines(const std::vector<cv::Point2f>& poly1, const std::vector<cv::Point2f>& poly2)
{
    std::vector<cv::Point2f> intersections;

    if (poly1.size() < 2 || poly2.size() < 2)
        return intersections;

    for (size_t i = 0; i + 1 < poly1.size(); ++i)
    {
        cv::Point2f A = poly1[i];
        cv::Point2f B = poly1[i + 1];

        for (size_t j = 0; j + 1 < poly2.size(); ++j)
        {
            cv::Point2f C = poly2[j];
            cv::Point2f D = poly2[j + 1];

            cv::Point2f P;
            if (segmentIntersection(A, B, C, D, P))
            {
                intersections.push_back(P);
            }
        }
    }

    return intersections;
}

// ------------------------------------------------------------
// Compute intersection between two line segments AB and CD.
// Returns true if they intersect and outputs the intersection point.
// ------------------------------------------------------------
bool OpenCVTools::segmentIntersection(const cv::Point2f& A, const cv::Point2f& B, const cv::Point2f& C, const cv::Point2f& D, cv::Point2f& out)
{
    cv::Point2f r = B - A;
    cv::Point2f s = D - C;

    float rxs = r.x * s.y - r.y * s.x;
    float qpxr = (C.x - A.x) * r.y - (C.y - A.y) * r.x;

    if (std::fabs(rxs) < std::numeric_limits<float>::epsilon())
    {
        // Lines are parallel or collinear
        return false;
    }

    float t = ((C.x - A.x) * s.y - (C.y - A.y) * s.x) / rxs;
    float u = qpxr / rxs;

    if (t >= 0 && t <= 1 && u >= 0 && u <= 1)
    {
        out = A + t * r;
        return true;
    }

    return false;
}

cv::Point2f OpenCVTools::normalize(const cv::Point2f& v)
{
    float len = std::sqrt(v.x*v.x + v.y*v.y);
    if (len == 0.0f)
        return cv::Point2f(0.0f, 0.0f);
    return cv::Point2f(v.x/len, v.y/len);
}

cv::Point3f OpenCVTools::normalize(const cv::Point3f& v)
{
    float len = std::sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
    if (len == 0.0f) return cv::Point3f(0,0,0);
    return cv::Point3f(v.x/len, v.y/len, v.z/len);
}


// Segment + Vertex Normals for polyline
// •   supports open/closed and left/right normal policies
// Segment normals
// •   Count = n-1 for open polylines
// •   Count = n for closed polylines
// •   Each normal is unit length and oriented left or right
// Vertex normals
// •   Endpoints (open polyline) get only one contributing segment
// •   Interior vertices get the average of adjacent segments
// •   Closed polylines get two contributions per vertex
// •   All vertex normals are normalized at the end

void OpenCVTools::computeNormals(const std::vector<cv::Point2f>& polyline, bool closed, bool leftNormals, std::vector<cv::Point2f>& segmentNormals, std::vector<cv::Point2f>& vertexNormals)
{
    segmentNormals.clear();
    vertexNormals.clear();

    const size_t n = polyline.size();
    if (n < 2)
        return;

    const size_t segmentCount = closed ? n : (n - 1);

    segmentNormals.resize(segmentCount);
    vertexNormals.assign(n, cv::Point2f(0.0f, 0.0f));

    // --- Compute segment normals ---
    for (size_t i = 0; i < segmentCount; ++i)
    {
        size_t i0 = i;
        size_t i1 = (i + 1);
        if (closed)
            i1 %= n;

        cv::Point2f d = polyline[i1] - polyline[i0];
        float len = std::sqrt(d.x*d.x + d.y*d.y);

        cv::Point2f segN(0.0f, 0.0f);
        if (len != 0.0f)
        {
            d.x /= len;
            d.y /= len;

            if (leftNormals)
                segN = cv::Point2f(-d.y, d.x);   // left-hand
            else
                segN = cv::Point2f(d.y, -d.x);   // right-hand
        }

        segmentNormals[i] = segN;

        // Accumulate into vertex normals
        vertexNormals[i0] += segN;
        vertexNormals[i1] += segN;
    }

    // --- Normalize vertex normals ---
    for (size_t i = 0; i < n; ++i)
        vertexNormals[i] = normalize(vertexNormals[i]);
}

// Deterministic Ray–Polyline Intersection (All Hits, Stable Ordering)
// This guarantees a total ordering.
// fully deterministic even for self intersecting polylines, with:
// •   A strict, explicit ordering policy
// •   A canonical tie breaking rule
// •   Stable sorting
// •   Epsilon based merging of duplicate intersection points
// •   Deterministic behavior across compilers, platforms, and floating point quirks
// Deterministic ordering rules
// 1.   Primary key: abs(t) (closest intersection in either direction)
// 2.   Secondary key: t (negative before positive when |t| ties)
// 3.   Tertiary key: segmentIndex (lower index first)
// 4.   Quaternary key: u parameter along the segment
// 5.   Duplicate merging: If two hits are within eps in both t and point position, merge them into a canonical hit

void OpenCVTools::intersectRayWithPolylineDeterministic(const std::vector<cv::Point2f>& polyline, const cv::Point2f& rayOrigin, const cv::Point2f& rayDir, bool closed, std::vector<OpenCVTools::RayHit>& outHits, float eps)
{
    outHits.clear();

    const size_t n = polyline.size();
    if (n < 2)
        return;

    // Normalize ray direction
    cv::Point2f d = rayDir;
    float dlen = std::sqrt(d.x*d.x + d.y*d.y);
    if (dlen == 0.0f)
        return;
    d.x /= dlen;
    d.y /= dlen;

    const size_t segmentCount = closed ? n : (n - 1);

    // --- Collect all raw intersections ---
    for (size_t i = 0; i < segmentCount; ++i)
    {
        size_t i0 = i;
        size_t i1 = (i + 1) % n;

        cv::Point2f p0 = polyline[i0];
        cv::Point2f p1 = polyline[i1];
        cv::Point2f s = p1 - p0;

        float det = d.x * (-s.y) - d.y * (-s.x);
        if (std::fabs(det) < 1e-12f)
            continue;

        cv::Point2f diff = p0 - rayOrigin;

        float t = (diff.x * (-s.y) - diff.y * (-s.x)) / det;
        float u = (d.x * diff.y - d.y * diff.x) / det;

        if (u >= 0.0f && u <= 1.0f)
        {
            RayHit hit;
            hit.t = t;
            hit.u = u;
            hit.segmentIndex = i;
            hit.point = rayOrigin + d * t;
            outHits.push_back(hit);
        }
    }

    if (outHits.empty())
        return;

    // --- Deterministic sort ---
    std::sort(outHits.begin(), outHits.end(),
              [&](const RayHit& a, const RayHit& b)
              {
                  float at = std::fabs(a.t);
                  float bt = std::fabs(b.t);

                  if (std::fabs(at - bt) > eps)
                      return at < bt;

                  if (std::fabs(a.t - b.t) > eps)
                      return a.t < b.t;

                  if (a.segmentIndex != b.segmentIndex)
                      return a.segmentIndex < b.segmentIndex;

                  if (std::fabs(a.u - b.u) > eps)
                      return a.u < b.u;

                  return false; // stable
              });

    // --- Merge duplicates deterministically ---
    std::vector<RayHit> merged;
    merged.reserve(outHits.size());

    merged.push_back(outHits[0]);

    for (size_t i = 1; i < outHits.size(); ++i)
    {
        const RayHit& prev = merged.back();
        const RayHit& curr = outHits[i];

        bool sameT = std::fabs(prev.t - curr.t) < eps;
        bool sameX = std::fabs(prev.point.x - curr.point.x) < eps;
        bool sameY = std::fabs(prev.point.y - curr.point.y) < eps;

        if (sameT && sameX && sameY)
        {
            // Merge: keep the canonical one (the earlier in sorted order)
            continue;
        }

        merged.push_back(curr);
    }

    outHits.swap(merged);
}


cv::Point3f OpenCVTools::rotateAroundAxis(const cv::Point3f& p, const cv::Point3f& axisPoint, const cv::Point3f& axisDirNorm, float angle)
{
    // Rodrigues' rotation formula
    cv::Point3f v = p - axisPoint;
    float c = std::cos(angle);
    float s = std::sin(angle);
    cv::Point3f k = axisDirNorm;

    cv::Point3f v_rot =
        v * c +
        k.cross(v) * s +
        k * (k.dot(v)) * (1.0f - c);

    return axisPoint + v_rot;
}

// Full Solid of Revolution Generator
// •    Optional caps (top and bottom)
// •    Vertex normals (smooth shading)
// •    UV coordinates (for texturing)
// •    Arbitrary axis of revolution
// •    OpenCV polyline input
// •    Triangle mesh output
// ✔ Side surface
// •    Full revolution
// •    Smooth normals
// •    UVs in [0,1] × [0,1]
// •    Deterministic triangle layout
// ✔ Caps (optional)
// •    Center vertex + ring
// •    Correct normal direction
// •    Circular UV mapping
// ✔ Axis of revolution
// Any axis defined by:
// cv::Point3f axisPoint;
// cv::Point3f axisDir;


OpenCVTools::Mesh OpenCVTools::revolvePolyline(const std::vector<cv::Point2f>& polyline2D, const cv::Point3f& axisPoint, const cv::Point3f& axisDir, int slices, bool capStart, bool capEnd)
{
    Mesh mesh;

    if (polyline2D.size() < 2 || slices < 3)
        return mesh;

    // Convert to 3D
    std::vector<cv::Point3f> polyline;
    polyline.reserve(polyline2D.size());
    for (auto& p : polyline2D)
        polyline.emplace_back(p.x, p.y, 0.0f);

    int n = polyline.size();
    cv::Point3f axisDirNorm = normalize(axisDir);
    float dtheta = 2.0f * float(M_PI) / float(slices);

    // Reserve memory
    mesh.vertices.reserve(n * slices + (capStart ? slices+1 : 0) + (capEnd ? slices+1 : 0));
    mesh.normals.reserve(mesh.vertices.capacity());
    mesh.uvs.reserve(mesh.vertices.capacity());

    // --- Generate side vertices, normals, UVs ---
    for (int s = 0; s < slices; ++s)
    {
        float angle = s * dtheta;
        float u = float(s) / float(slices - 1);

        for (int i = 0; i < n; ++i)
        {
            cv::Point3f p = polyline[i];
            cv::Point3f pr = rotateAroundAxis(p, axisPoint, axisDirNorm, angle);

            mesh.vertices.push_back(pr);

            // Normal = derivative wrt angle (tangent around axis)
            cv::Point3f tangent = rotateAroundAxis(p, axisPoint, axisDirNorm, angle + 0.001f) - pr;
            mesh.normals.push_back(normalize(tangent));

            float v = float(i) / float(n - 1);
            mesh.uvs.emplace_back(u, v);
        }
    }

    // --- Generate side triangles ---
    for (int s = 0; s < slices; ++s)
    {
        int sNext = (s + 1) % slices;

        for (int i = 0; i < n - 1; ++i)
        {
            int i0 = s * n + i;
            int i1 = s * n + (i + 1);
            int i2 = sNext * n + i;
            int i3 = sNext * n + (i + 1);

            mesh.triangles.emplace_back(i0, i2, i1);
            mesh.triangles.emplace_back(i1, i2, i3);
        }
    }

    // --- Caps (optional) ---
    auto addCap = [&](bool atStart)
    {
        int baseIndex = mesh.vertices.size();
        int ringStart = atStart ? 0 : (n - 1);

        // Center vertex
        cv::Point3f center = polyline[ringStart];
        center = rotateAroundAxis(center, axisPoint, axisDirNorm, 0);
        mesh.vertices.push_back(center);

        cv::Point3f capNormal = atStart ? -axisDirNorm : axisDirNorm;
        mesh.normals.push_back(capNormal);
        mesh.uvs.emplace_back(0.5f, 0.5f);

        int centerIndex = baseIndex;

        // Ring vertices
        for (int s = 0; s < slices; ++s)
        {
            float angle = s * dtheta;
            cv::Point3f p = polyline[ringStart];
            cv::Point3f pr = rotateAroundAxis(p, axisPoint, axisDirNorm, angle);

            mesh.vertices.push_back(pr);
            mesh.normals.push_back(capNormal);

            float u = 0.5f + 0.5f * std::cos(angle);
            float v = 0.5f + 0.5f * std::sin(angle);
            mesh.uvs.emplace_back(u, v);
        }

        // Triangles
        for (int s = 0; s < slices; ++s)
        {
            int sNext = (s + 1) % slices;
            int i0 = centerIndex;
            int i1 = baseIndex + 1 + s;
            int i2 = baseIndex + 1 + sNext;

            if (atStart)
                mesh.triangles.emplace_back(i0, i2, i1);
            else
                mesh.triangles.emplace_back(i0, i1, i2);
        }
    };

    if (capStart) addCap(true);
    if (capEnd)   addCap(false);

    return mesh;
}

// Produce an OBJ formatted string from a mesh. It follows the OBJ spec precisely:
// •    v → vertex
// •    vt → texture coordinate
// •    vn → normal
// •    f → face (1 indexed, with v/vt/vn triplets)
// It works even if UVs or normals are missing.

std::string OpenCVTools::meshToOBJ(const OpenCVTools::Mesh& mesh, const std::string& objectName)
{
    std::ostringstream out;

    out << "o " << objectName << "\n";

    // --- Vertices ---
    for (const auto& v : mesh.vertices)
        out << "v " << v.x << " " << v.y << " " << v.z << "\n";

    // --- UVs ---
    bool hasUV = !mesh.uvs.empty();
    if (hasUV)
    {
        for (const auto& uv : mesh.uvs)
            out << "vt " << uv.x << " " << uv.y << "\n";
    }

    // --- Normals ---
    bool hasNormals = !mesh.normals.empty();
    if (hasNormals)
    {
        for (const auto& n : mesh.normals)
            out << "vn " << n.x << " " << n.y << " " << n.z << "\n";
    }

    // --- Faces ---
    // OBJ is 1-indexed
    for (const auto& tri : mesh.triangles)
    {
        int i0 = tri[0] + 1;
        int i1 = tri[1] + 1;
        int i2 = tri[2] + 1;

        if (hasUV && hasNormals)
        {
            out << "f "
                << i0 << "/" << i0 << "/" << i0 << " "
                << i1 << "/" << i1 << "/" << i1 << " "
                << i2 << "/" << i2 << "/" << i2 << "\n";
        }
        else if (hasUV)
        {
            out << "f "
                << i0 << "/" << i0 << " "
                << i1 << "/" << i1 << " "
                << i2 << "/" << i2 << "\n";
        }
        else if (hasNormals)
        {
            out << "f "
                << i0 << "//" << i0 << " "
                << i1 << "//" << i1 << " "
                << i2 << "//" << i2 << "\n";
        }
        else
        {
            out << "f "
                << i0 << " "
                << i1 << " "
                << i2 << "\n";
        }
    }

    return out.str();
}

bool OpenCVTools::subtractBackground(const std::string& inputPath, const std::string& outputPath)
{
    cv::VideoCapture cap(inputPath);
    if (!cap.isOpened()) {
        std::cerr << "Error: Cannot open input video: " << inputPath << std::endl;
        return false;
    }

    int width  = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
    int height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
    double fps = cap.get(cv::CAP_PROP_FPS);

    cv::Size size(width, height);
    bool isColor = false;
    // cv::VideoWriter writer;
    // OpenCVTools::WriterResult result = openWithFallback(writer, outputPath, fps, size, isColor);

    cv::VideoWriter writer(outputPath, cv::VideoWriter::fourcc('X','V','I','D'), fps, size, isColor);

    if (!writer.isOpened()) {
        std::cerr << "Error: Cannot open output video: " << outputPath << std::endl;
        return false;
    }

    // Create background subtractor (MOG2 is robust and widely supported)
    cv::Ptr<cv::BackgroundSubtractor> bg =
        cv::createBackgroundSubtractorMOG2(/*history=*/500,
                                           /*varThreshold=*/16,
                                           /*detectShadows=*/false);

    cv::Mat frame, fgMask;

    while (true) {
        if (!cap.read(frame) || frame.empty())
            break;

        // Apply background subtraction
        bg->apply(frame, fgMask);

        // Write mask to output
        writer.write(fgMask);
    }

    return true;
}

OpenCVTools::WriterResult OpenCVTools::openWithFallback(cv::VideoWriter& writer, std::string outputPath, double fps, cv::Size size, bool isColor)
{
    // Ordered fallback list
    std::vector<CodecAttempt> codecs = {
        {"H.264 (avc1)", cv::VideoWriter::fourcc('a','v','c','1'), ".mp4"},
        {"H.264 (H264)", cv::VideoWriter::fourcc('H','2','6','4'), ".mp4"},
        {"XVID",         cv::VideoWriter::fourcc('X','V','I','D'), ".avi"},
        {"MJPEG",        cv::VideoWriter::fourcc('M','J','P','G'), ".avi"},
        {"FFV1",         cv::VideoWriter::fourcc('F','F','V','1'), ".mkv"},
        {"HuffYUV",      cv::VideoWriter::fourcc('H','F','Y','U'), ".avi"}
    };

    WriterResult result;

    // Validate directory
    std::filesystem::path parent = std::filesystem::path(outputPath).parent_path();
    if (!parent.empty() && !std::filesystem::exists(parent)) {
        result.message = "Output directory does not exist: " + parent.string();
        return result;
    }

    // Try each codec in order
    for (const auto& c : codecs) {
        std::filesystem::path attemptPath = outputPath;
        attemptPath.replace_extension(c.containerHint);

        std::cout << "Trying codec: " << c.name
                  << " → " << attemptPath << std::endl;

        bool opened = writer.open(attemptPath.string(), c.fourcc, fps, size, isColor);

        if (!opened) {
            std::cout << "  Failed to open writer for " << c.name << std::endl;
            continue;
        }

        // Try writing a dummy frame
        cv::Mat dummy(size, isColor ? CV_8UC3 : CV_8UC1, cv::Scalar(0));
        try {
            writer.write(dummy);
        } catch (...) {
            std::cout << "  Codec " << c.name
                      << " opened but failed to encode a test frame" << std::endl;
            continue;
        }

        // Success
        result.ok = true;
        result.codecUsed = c.name;
        result.message = "Successfully opened writer using codec: " + c.name +
                         " → " + attemptPath.string();
        return result;
    }

    result.message = "All codec attempts failed. Check OpenCV build info and backend support.";
    return result;
}

// Resolve a 1-based (positive) or negative (relative) OBJ index into a
// 0-based index into a pool of the given size.  Returns false on failure.
bool OpenCVTools::resolveIndex(int raw, int poolSize, int& out)
{
    if (raw == 0) return false;
    int idx = (raw > 0) ? raw - 1 : poolSize + raw;
    if (idx < 0 || idx >= poolSize) return false;
    out = idx;
    return true;
}

// Split "v", "v/t", "v//n", or "v/t/n" into component raw OBJ indices.
// Missing components are left as 0.
bool OpenCVTools::parseFaceVertex(const std::string& token, int& v, int& t, int& n)
{
    v = t = n = 0;
    std::istringstream ss(token);
    std::string part;

    // position
    if (!std::getline(ss, part, '/')) return false;
    if (part.empty()) return false;
    try { v = std::stoi(part); } catch (...) { return false; }

    if (ss.eof()) return true;

    // uv (may be empty for "v//n")
    std::getline(ss, part, '/');
    if (!part.empty())
    {
        try { t = std::stoi(part); } catch (...) { return false; }
    }

    if (ss.eof()) return true;

    // normal
    std::getline(ss, part);
    if (!part.empty())
    {
        try { n = std::stoi(part); } catch (...) { return false; }
    }
    return true;
}

OpenCVTools::ObjError OpenCVTools::loadObj(const std::string& path, Mesh& mesh)
{
    std::ifstream file(path);
    if (!file.is_open())
        return ObjError::FILE_NOT_FOUND;

    mesh = {};

    // Raw OBJ pools (all 0-based after parsing, still per-attribute).
    std::vector<cv::Point3f> objV, objN;
    std::vector<cv::Point2f> objT;

    // Map (vIdx, tIdx, nIdx) → unified output vertex index.
    // We use -1 to mean "absent" for t and n.
    using Key = std::tuple<int,int,int>;
    struct KeyHash {
        size_t operator()(const Key& k) const {
            size_t h = std::hash<int>{}(std::get<0>(k));
            h ^= std::hash<int>{}(std::get<1>(k)) + 0x9e3779b9 + (h<<6) + (h>>2);
            h ^= std::hash<int>{}(std::get<2>(k)) + 0x9e3779b9 + (h<<6) + (h>>2);
            return h;
        }
    };
    std::unordered_map<Key, int, KeyHash> vertexCache;

    // Per-face polygon corners before fan-triangulation.
    std::vector<int> faceCorners;

    std::string line;
    int lineNum = 0;

    while (std::getline(file, line))
    {
        ++lineNum;

        // Strip comments and leading/trailing whitespace.
        auto commentPos = line.find('#');
        if (commentPos != std::string::npos)
            line.erase(commentPos);

        std::istringstream ss(line);
        std::string keyword;
        if (!(ss >> keyword) || keyword.empty())
            continue;

        if (keyword == "v")
        {
            float x, y, z;
            if (!(ss >> x >> y >> z))
                return ObjError::MALFORMED_DATA;
            objV.push_back({x, y, z});
        }
        else if (keyword == "vn")
        {
            float x, y, z;
            if (!(ss >> x >> y >> z))
                return ObjError::MALFORMED_DATA;
            objN.push_back({x, y, z});
        }
        else if (keyword == "vt")
        {
            float u, v;
            if (!(ss >> u >> v))
                return ObjError::MALFORMED_DATA;
            objT.push_back({u, v});
        }
        else if (keyword == "f")
        {
            faceCorners.clear();
            std::string token;

            while (ss >> token)
            {
                int rawV, rawT, rawN;
                if (!parseFaceVertex(token, rawV, rawT, rawN))
                    return ObjError::MALFORMED_DATA;

                int vi, ti = -1, ni = -1;
                if (!resolveIndex(rawV, static_cast<int>(objV.size()), vi))
                    return ObjError::INDEX_OUT_OF_RANGE;
                if (rawT != 0 && !resolveIndex(rawT, static_cast<int>(objT.size()), ti))
                    return ObjError::INDEX_OUT_OF_RANGE;
                if (rawN != 0 && !resolveIndex(rawN, static_cast<int>(objN.size()), ni))
                    return ObjError::INDEX_OUT_OF_RANGE;

                Key key{vi, ti, ni};
                auto [it, inserted] = vertexCache.emplace(key, static_cast<int>(mesh.vertices.size()));
                if (inserted)
                {
                    mesh.vertices.push_back(objV[vi]);
                    mesh.normals.push_back(ni >= 0 ? objN[ni] : cv::Point3f{0, 0, 0});
                    mesh.uvs.push_back(ti >= 0 ? objT[ti] : cv::Point2f{0, 0});
                }
                faceCorners.push_back(it->second);
            }

            // Fan-triangulate (works for convex polygons; sufficient for most OBJ).
            if (faceCorners.size() < 3)
                return ObjError::MALFORMED_DATA;
            for (size_t i = 1; i + 1 < faceCorners.size(); ++i)
                mesh.triangles.push_back({faceCorners[0], faceCorners[i], faceCorners[i + 1]});
        }
        // Silently ignore: mtllib, usemtl, o, g, s, l, etc.
    }

    return ObjError::OK;
}