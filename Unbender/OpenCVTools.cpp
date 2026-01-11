#include "OpenCVTools.h"

#include <QImage>
#include <QPainterPath>
#include <QDebug>

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
// •	Within vertexTolerance → split at that vertex
// •	No new point inserted
// •	Polyline starts at that vertex
// Otherwise:
// •	Compute closest point on the closest segment
// •	Insert that point
// •	Rotate so it becomes the start
// •	Polyline becomes open
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


