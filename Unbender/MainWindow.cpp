#include "MainWindow.h"
#include "./ui_MainWindow.h"

#include "GraphicsView.h"
#include "MeshViewWidget.h"
#include "OpenCVTools.h"
#include "MarkerItem.h"

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QFileDialog>
#include <QImageReader>
#include <QMenuBar>
#include <QAction>
#include <QWheelEvent>
#include <QSettings>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QSplitter>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_ui(new Ui::MainWindow)
{
    m_ui->setupUi(this);

    // Central widget container
    QWidget* central = new QWidget(this);
    setCentralWidget(central);

    // Layout for the central widget
    QHBoxLayout* layout = new QHBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);

    // Horizontal splitter
    m_splitter = new QSplitter(Qt::Horizontal, central);
    layout->addWidget(m_splitter);

    // Left side: customised GraphicsView
    QGraphicsScene* scene = new QGraphicsScene(this);
    GraphicsView* m_view = new GraphicsView(scene);

    // Right side: your custom OpenGL widget
    MeshViewWidget* meshView = new MeshViewWidget;

    // Add widgets to splitter
    m_splitter->addWidget(m_view);
    m_splitter->addWidget(meshView);

    // Optional: set initial sizes
    m_splitter->setStretchFactor(0, 1);  // graphics view grows
    m_splitter->setStretchFactor(1, 1);  // mesh view grows

    connect(m_ui->actionOpen, &QAction::triggered, this, &MainWindow::openImage);
    connect(m_ui->actionStraighten, &QAction::triggered, this, &MainWindow::straighten);
    connect(m_ui->actionStraightenMore, &QAction::triggered, this, &MainWindow::straightenMore);
    connect(m_ui->actionQuit, &QAction::triggered, this, &MainWindow::close);

    setWindowTitle("Image Viewer");

    readSettings();
    updateUI();

}

MainWindow::~MainWindow()
{
    writeSettings();
    delete m_ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_unsavedChanges) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Unsaved Changes", "You have unsaved changes. Do you want to save before quitting?", QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

        if (reply == QMessageBox::Yes)
        {
            // Call your save routine
            saveDocument();
            event->accept();
        }
        else if (reply == QMessageBox::No)
        {
            event->accept();  // quit without saving
        } else {
            event->ignore();  // cancel close
        }
    }
    else
    {
        event->accept();  // no unsaved changes, just quit
    }
}

void MainWindow::openImage()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Open Image", m_filePath, "Images (*.png *.jpg *.jpeg *.bmp *.gif)");

    if (!fileName.isEmpty())
    {
        m_filePath = fileName;
        QImageReader reader(fileName);
        reader.setAutoTransform(true);
        QImage image = reader.read();

        if (!image.isNull())
        {
            // cv::Mat mat = OpenCVTools::convertQImageToMat(image);
            // QImage newImage = OpenCVTools::convertMatToQImage(mat);
            // QGraphicsPixmapItem *item = new QGraphicsPixmapItem(QPixmap::fromImage(newImage));
            QGraphicsPixmapItem *item = new QGraphicsPixmapItem(QPixmap::fromImage(image));
            m_view->clear();
            m_view->setImage(item);
            m_view->fitInView(m_view->scene()->itemsBoundingRect(), Qt::KeepAspectRatio);
            // I want to stadardise on the endedness independent QImage formats
            switch (image.format())
            {
            // these formats are used unchanged because they are endian indepenndent
            case QImage::Format_Grayscale8:
            case QImage::Format_RGB888:
            case QImage::Format_RGBA8888:
                m_image = std::make_unique<QImage>(image);
                break;
            // these formats need to be converted
            case QImage::QImage::Format_Grayscale16:
                m_image = std::make_unique<QImage>(image.convertToFormat(QImage::Format_Grayscale8));
                break;
            case QImage::QImage::Format_RGB32:
                m_image = std::make_unique<QImage>(image.convertToFormat(QImage::Format_RGB888));
                break;
            case QImage::QImage::Format_ARGB32:
                m_image = std::make_unique<QImage>(image.convertToFormat(QImage::Format_RGBA8888));
                break;
            // everything else to Format_RGB888
            default:
                m_image = std::make_unique<QImage>(image.convertToFormat(QImage::Format_RGB888));
                break;
            }
        }
    }
    updateUI();
}

void MainWindow::saveDocument()
{
    updateUI();
}

void MainWindow::straighten()
{
    cv::Mat img = OpenCVTools::convertQImageToMat(*m_image);
    cv::Mat thresh = OpenCVTools::thresholdImage(img, 100, true);
    QImage thresholdImage = OpenCVTools::convertMatToQImage(thresh);
    m_view->setImage(new QGraphicsPixmapItem(QPixmap::fromImage(thresholdImage)));
    std::vector<cv::Point> outline = OpenCVTools::polylineFromBinaryImage(thresh);

    std::vector<cv::Point2f> outlinef;
    outlinef.reserve(outline.size());
    for (const auto& p : outline) { outlinef.emplace_back(static_cast<float>(p.x), static_cast<float>(p.y)); }
    MarkerItem *p1 = m_view->position1();
    cv::Point2f userPoint1(p1->pos().x(), p1->pos().y());
    float vertexTolerance = 0.001;
    std::vector<cv::Point2f> openOutline = OpenCVTools::splitClosedPolylineRobust(outlinef, userPoint1, vertexTolerance);
    MarkerItem *p2 = m_view->position2();
    cv::Point2f userPoint2(p2->pos().x(), p2->pos().y());
    OpenCVTools::splitOpenPolylineRobust(openOutline, userPoint2, vertexTolerance, m_polyA, m_polyB);
    std::reverse(m_polyB.begin(), m_polyB.end()); // I want both polylines to start at userPoint1

    QGraphicsPathItem *item;
    item = new QGraphicsPathItem(OpenCVTools::convertPolylineToQPainterPath(m_polyA, false));
    item->setPen(QPen(Qt::cyan, 2));
    item->setBrush(Qt::NoBrush);
    m_view->addExtraItem(item);
    item = new QGraphicsPathItem(OpenCVTools::convertPolylineToQPainterPath(m_polyB, false));
    item->setPen(QPen(Qt::magenta, 2));
    item->setBrush(Qt::NoBrush);
    m_view->addExtraItem(item);

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
        item = new QGraphicsPathItem(OpenCVTools::convertPolylineToQPainterPath(stick, false));
        item->setPen(QPen(Qt::darkYellow, 1));
        item->setBrush(Qt::NoBrush);
        m_view->addExtraItem(item);
    }

    item = new QGraphicsPathItem(OpenCVTools::convertPolylineToQPainterPath(m_centreLine, false));
    item->setPen(QPen(Qt::yellow, 2));
    item->setBrush(Qt::NoBrush);
    m_view->addExtraItem(item);

    updateUI();
}

void MainWindow::straightenMore()
{
    m_view->clearExtrasItems();
    QGraphicsPathItem *item;
    item = new QGraphicsPathItem(OpenCVTools::convertPolylineToQPainterPath(m_polyA, false));
    item->setPen(QPen(Qt::cyan, 2));
    item->setBrush(Qt::NoBrush);
    m_view->addExtraItem(item);
    item = new QGraphicsPathItem(OpenCVTools::convertPolylineToQPainterPath(m_polyB, false));
    item->setPen(QPen(Qt::magenta, 2));
    item->setBrush(Qt::NoBrush);
    m_view->addExtraItem(item);

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
        item = new QGraphicsPathItem(OpenCVTools::convertPolylineToQPainterPath(stick, false));
        item->setPen(QPen(Qt::darkGreen, 1));
        item->setBrush(Qt::NoBrush);
        m_view->addExtraItem(item);
    }
    newCentreLine.push_back(m_centreLine.back());

    item = new QGraphicsPathItem(OpenCVTools::convertPolylineToQPainterPath(newCentreLine, false));
    item->setPen(QPen(Qt::green, 1));
    item->setBrush(Qt::NoBrush);
    m_view->addExtraItem(item);

    m_centreLine = newCentreLine;

    updateUI();
}

void MainWindow::updateUI()
{
    m_ui->actionOpen->setEnabled(true);
    m_ui->actionStraighten->setEnabled(m_image != 0 && m_view->position1() && m_view->position2());
    m_ui->actionStraightenMore->setEnabled(m_image != 0 && m_view->position1() && m_view->position2() && m_polyA.size() && m_polyB.size() && m_centreLine.size());
}

void MainWindow::readSettings()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "AnimalSimulationLaboratory", "Unbender");
    m_filePath = settings.value("lastFileOpened", "").toString();
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
    m_splitter->restoreState(settings.value("splitterState").toByteArray());
}

void MainWindow::writeSettings()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "AnimalSimulationLaboratory", "Unbender");
    settings.setValue("lastFileOpened", m_filePath);
    settings.setValue("splitterState", m_splitter->saveState());
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    settings.sync();
}

