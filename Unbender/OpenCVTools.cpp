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
            return cv::Mat(img.height(), img.width(), CV_8UC1, const_cast<uchar*>(img.bits()), img.bytesPerLine()).clone();
        }

        // RGB (QImage::Format_RGB888) → CV_8UC3 (needs channel swap)
        if (img.format() == QImage::Format_RGB888)
        {
            cv::Mat mat(img.height(), img.width(), CV_8UC3, const_cast<uchar*>(img.bits()), img.bytesPerLine());
            cv::Mat matBGR;
            cv::cvtColor(mat, matBGR, cv::COLOR_RGB2BGR); // Qt uses RGB, OpenCV expects BGR
            return matBGR;
        }

        // RGB (QImage::Format_RGBA8888) → CV_8UC4 (needs channel swap and throws away the unused alpha channel)
        if (img.format() == QImage::Format_RGBA8888)
        {
            cv::Mat mat(img.height(), img.width(), CV_8UC4, const_cast<uchar*>(img.bits()), img.bytesPerLine());
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
    // std::string s = matInfoToString(img, "input"); qDebug() << s.c_str();
    // cv::imwrite("C:/Scratch/incoming.png", img);
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
            // cv::imwrite("C:/Scratch/grey.png", grey);
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

// For each pixel (B, G, R, A) in BGRA:
// [ B' = \frac{A}{255} \cdot B + \left(1 - \frac{A}{255}\right) \cdot 255 ]
// [ G' = \frac{A}{255} \cdot G + \left(1 - \frac{A}{255}\right) \cdot 255 ]
// [ R' = \frac{A}{255} \cdot R + \left(1 - \frac{A}{255}\right) \cdot 255 ]
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

cv::Point2f OpenCVTools::closestPointOnSegment(const cv::Point2f& p, const cv::Point2f& a, const cv::Point2f& b, float& tOut)
{
    cv::Point2f ab = b - a;
    float ab2 = ab.dot(ab);
    if (ab2 == 0.0f)
    {
        tOut = 0.0f;
        return a; // degenerate segment
    }

    float t = (p - a).dot(ab) / ab2;
    t = std::max(0.0f, std::min(1.0f, t));
    tOut = t;
    return a + t * ab;
}

void OpenCVTools::splitPolyline(const std::vector<cv::Point2f>& poly, const cv::Point2f& userPoint, bool isClosed, std::vector<cv::Point2f>& outA, std::vector<cv::Point2f>& outB)
{
    int n = (int)poly.size();
    if (n < 2)
        return;

    float bestDist2 = std::numeric_limits<float>::max();
    int bestIndex = -1;
    float bestT = 0.0f;
    cv::Point2f bestPoint;

    int segCount = isClosed ? n : n - 1;

    // Find closest segment (including closing segment if closed)
    for (int i = 0; i < segCount; i++)
    {
        int j = (i + 1) % n; // wraps around for closed polylines
        float t;
        cv::Point2f cp = closestPointOnSegment(userPoint, poly[i], poly[j], t);
        float d2 = (cp - userPoint).dot(cp - userPoint);
        if (d2 < bestDist2)
        {
            bestDist2 = d2;
            bestIndex = i;
            bestT = t;
            bestPoint = cp;
        }
    }

    // Build first polyline
    outA.clear();
    for (int i = 0; i <= bestIndex; i++)
        outA.push_back(poly[i]);
    outA.push_back(bestPoint);

    // Build second polyline
    outB.clear();
    outB.push_back(bestPoint);

    int next = (bestIndex + 1) % n;
    if (!isClosed)
    {
        // Open polyline: just continue to the end
        for (int i = bestIndex + 1; i < n; i++)
            outB.push_back(poly[i]);
    }
    else
    {
        // Closed polyline: wrap around until we reach bestIndex again
        for (int i = next; i != bestIndex; i = (i + 1) % n)
            outB.push_back(poly[i]);
    }
}
