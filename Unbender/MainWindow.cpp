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
#include <QStyle>
#include <QToolBar>

#include <fstream>


MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) , m_ui(new Ui::MainWindow)
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
    m_view = new GraphicsView(scene);

    // Right side: your custom OpenGL widget
    m_meshView = new MeshViewWidget(this);

    // Add widgets to splitter
    m_splitter->addWidget(m_view);
    m_splitter->addWidget(m_meshView);

    // Force even split
    m_splitter->setSizes({1, 1});

    // Create actions
    auto *openAction = new QAction(style()->standardIcon(QStyle::SP_DialogOpenButton), tr("Open"), this);
    openAction->setObjectName("openAction");
    auto *saveAction = new QAction(style()->standardIcon(QStyle::SP_DialogSaveButton), tr("Save"), this);
    saveAction->setObjectName("saveAction");
    auto *quitAction = new QAction(style()->standardIcon(QStyle::SP_DialogCloseButton), tr("Quit"), this);
    quitAction->setObjectName("quitAction");

    auto *straightenAction = new QAction(tr("Straighten"), this);
    straightenAction->setObjectName("straightenAction");
    auto *straightenMoreAction = new QAction(tr("Straighten More"), this);
    straightenMoreAction->setObjectName("straightenMoreAction");
    auto *backgroundSubtractVideo = new QAction(tr("Background Subtract Video..."), this);
    backgroundSubtractVideo->setObjectName("backgroundSubtractVideo");

    // Create toolbar
    auto *toolbar = addToolBar(tr("Main Toolbar"));
    toolbar->setObjectName("mainToolbar");  // useful for saving/restoring state

    // Add actions
    toolbar->addAction(openAction);
    toolbar->addAction(saveAction);
    toolbar->addSeparator();
    toolbar->addAction(quitAction);

    // Create menus
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(openAction);
    fileMenu->addAction(saveAction);
    fileMenu->addSeparator();
    fileMenu->addAction(quitAction);

    QMenu *actionMenu = menuBar()->addMenu(tr("&Action"));
    actionMenu->addAction(straightenAction);
    actionMenu->addAction(straightenMoreAction);
    actionMenu->addSeparator();
    actionMenu->addAction(backgroundSubtractVideo);

    connect(openAction, &QAction::triggered, this, &MainWindow::openImage);
    connect(saveAction, &QAction::triggered, this, &MainWindow::saveDocument);
    connect(quitAction, &QAction::triggered, this, &MainWindow::close);
    connect(straightenAction, &QAction::triggered, this, &MainWindow::straighten);
    connect(straightenMoreAction, &QAction::triggered, this, &MainWindow::straightenMore);
    connect(backgroundSubtractVideo, &QAction::triggered, this, &MainWindow::backgroundSubtractVideo);

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
    if (m_unsavedChanges)
    {
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
        }
        else
        {
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
    QString fileName = QFileDialog::getOpenFileName(this, "Open Image", m_filePath, "Images (*.png *.jpg *.jpeg *.bmp *.gif);;Any File (*.* *)");

    if (!fileName.isEmpty())
    {
        m_filePath = fileName;
        QImageReader reader(fileName);
        reader.setAutoTransform(true);
        QImage image = reader.read();

        if (!image.isNull())
        {
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
    m_findCentreLine.setImg(OpenCVTools::convertQImageToMat(*m_image));

    MarkerItem *p1 = m_view->position1();
    m_findCentreLine.setUserPoint1(cv::Point2f(p1->pos().x(), p1->pos().y()));
    MarkerItem *p2 = m_view->position2();
    m_findCentreLine.setUserPoint2(cv::Point2f(p2->pos().x(), p2->pos().y()));

    m_findCentreLine.straighten();

    QImage thresholdImage = OpenCVTools::convertMatToQImage(m_findCentreLine.thresh());
    m_view->setImage(new QGraphicsPixmapItem(QPixmap::fromImage(thresholdImage)));

    QGraphicsPathItem *item;
    item = new QGraphicsPathItem(OpenCVTools::convertPolylineToQPainterPath(m_findCentreLine.polyA(), false));
    item->setPen(QPen(Qt::cyan, 2));
    item->setBrush(Qt::NoBrush);
    m_view->addExtraItem(item);
    item = new QGraphicsPathItem(OpenCVTools::convertPolylineToQPainterPath(m_findCentreLine.polyB(), false));
    item->setPen(QPen(Qt::magenta, 2));
    item->setBrush(Qt::NoBrush);
    m_view->addExtraItem(item);

    auto stickList = m_findCentreLine.stickList();
    for (size_t i = 0; i < stickList.size(); ++i)
    {
        item = new QGraphicsPathItem(OpenCVTools::convertPolylineToQPainterPath(stickList[i], false));
        item->setPen(QPen(Qt::darkYellow, 1));
        item->setBrush(Qt::NoBrush);
        m_view->addExtraItem(item);
    }

    item = new QGraphicsPathItem(OpenCVTools::convertPolylineToQPainterPath(m_findCentreLine.centreLine(), false));
    item->setPen(QPen(Qt::yellow, 2));
    item->setBrush(Qt::NoBrush);
    m_view->addExtraItem(item);

    updateUI();
}

void MainWindow::straightenMore()
{
    m_findCentreLine.straightenMore();

    m_view->clearExtrasItems();
    QGraphicsPathItem *item;
    item = new QGraphicsPathItem(OpenCVTools::convertPolylineToQPainterPath(m_findCentreLine.polyA(), false));
    item->setPen(QPen(Qt::cyan, 2));
    item->setBrush(Qt::NoBrush);
    m_view->addExtraItem(item);
    item = new QGraphicsPathItem(OpenCVTools::convertPolylineToQPainterPath(m_findCentreLine.polyB(), false));
    item->setPen(QPen(Qt::magenta, 2));
    item->setBrush(Qt::NoBrush);
    m_view->addExtraItem(item);

    auto stickList = m_findCentreLine.stickList();
    for (size_t i = 0; i < stickList.size(); ++i)
    {
        item = new QGraphicsPathItem(OpenCVTools::convertPolylineToQPainterPath(stickList[i], false));
        item->setPen(QPen(Qt::darkGreen, 1));
        item->setBrush(Qt::NoBrush);
        m_view->addExtraItem(item);
    }


    item = new QGraphicsPathItem(OpenCVTools::convertPolylineToQPainterPath(m_findCentreLine.centreLine(), false));
    item->setPen(QPen(Qt::green, 1));
    item->setBrush(Qt::NoBrush);
    m_view->addExtraItem(item);

    m_findCentreLine.createStraightVersion();
    auto mesh = m_findCentreLine.straightMesh();
    std::string objVersion = OpenCVTools::meshToOBJ(mesh, "straight_mesh");
    std::ofstream("C:\\Scratch\\output.obj") << objVersion;

    updateUI();
}


void MainWindow::updateUI()
{
    findChild<QAction*>("openAction")->setEnabled(true);
    findChild<QAction*>("quitAction")->setEnabled(true);
    findChild<QAction*>("saveAction")->setEnabled(false);
    findChild<QAction*>("straightenAction")->setEnabled(m_image != 0 && m_view->position1() && m_view->position2());
    findChild<QAction*>("straightenMoreAction")->setEnabled(m_image != 0 && m_view->position1() && m_view->position2() && m_findCentreLine.polyA().size() && m_findCentreLine.polyB().size() && m_findCentreLine.centreLine().size());
    findChild<QAction*>("backgroundSubtractVideo")->setEnabled(true);

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

void MainWindow::backgroundSubtractVideo()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Open Movie", m_filePath, "Movies (*.mp4 *.avi *.mov);;Any File (*.* *)");

    if (!fileName.isEmpty())
    {
        std::filesystem::path inputPath = fileName.toStdString();
        std::filesystem::path outputPath = inputPath;
        outputPath.replace_filename(inputPath.stem().string() + "_no_bg.mp4");
        OpenCVTools::subtractBackground(inputPath.string(), outputPath.string());
    }
}
