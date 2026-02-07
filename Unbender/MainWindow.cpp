#include "MainWindow.h"
#include "./ui_MainWindow.h"

#include "GraphicsView.h"
#include "MeshViewWidget.h"
#include "OpenCVTools.h"
#include "MarkerItem.h"
#include "Sidebar.h"
#include "FourPaneViewport.h"

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
#include <QLineEdit>
#include <QWidgetAction>
#include <QSpinBox>
#include <QLabel>
#include <QCheckBox>

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
    m_splitter->setHandleWidth(10);   // default is usually 1–3 px
    layout->addWidget(m_splitter);

    // Left side: the sidebar
    m_sidebar = new Sidebar(this);
    m_sidebar->addPathEditWidget("Frames", "framesFolder", PathEditWidget::DirectoryMode, "");
    m_sidebar->addPathEditWidget("Masks", "masksFolder", PathEditWidget::DirectoryMode, "");
    m_sidebar->addPathEditWidget("Output", "outputFolder", PathEditWidget::DirectoryMode, "");
    m_sidebar->addSpinBox("Threshold", "threshold", 0, 255, 127);
    m_sidebar->addCheckBox("Invert", "invert", 0);
    m_sidebar->addSpacer();

    // Right side: the four pane viewport

    // 3 of the views are image viewers
    QGraphicsScene *scene;
    scene = new QGraphicsScene(this);
    m_originalView = new GraphicsView(scene);
    scene = new QGraphicsScene(this);
    m_maskView = new GraphicsView(scene);
    scene = new QGraphicsScene(this);
    m_processedView = new GraphicsView(scene);

    // 1 view is the 3D mesh viewer
    m_meshView = new MeshViewWidget(this);

    m_fourPaneViewport = new FourPaneViewport(m_originalView, m_maskView, m_processedView, m_meshView, this);

    // Add widgets to splitter
    m_splitter->addWidget(m_sidebar);
    m_splitter->addWidget(m_fourPaneViewport);

    // Force even split
    m_splitter->setSizes({1, 4});

    // Create actions
    auto *quitAction = new QAction(style()->standardIcon(QStyle::SP_TitleBarCloseButton), tr("Quit"), this);

    m_straightenAction = new QAction(tr("Straighten"), this);
    m_straightenMoreAction = new QAction(tr("Straighten More"), this);

    m_firstImage = new QAction(style()->standardIcon(QStyle::SP_MediaSkipBackward), tr("First Image"), this);
    m_lastImage = new QAction(style()->standardIcon(QStyle::SP_MediaSkipForward), tr("Last Images"), this);
    m_nextImage = new QAction(style()->standardIcon(QStyle::SP_MediaSeekForward), tr("Next Image"), this);
    m_previousImage = new QAction(style()->standardIcon(QStyle::SP_MediaSeekBackward), tr("Previous Images"), this);

    // Create toolbar
    auto *toolbar = addToolBar(tr("Main Toolbar"));
    toolbar->setObjectName("mainToolbar");  // useful for saving/restoring state
    QWidgetAction *widgetAction;

    // Add actions
    toolbar->addAction(m_firstImage);
    toolbar->addAction(m_previousImage);
    toolbar->addAction(m_nextImage);
    toolbar->addAction(m_lastImage);

    // Create menus
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(quitAction);

    QMenu *actionMenu = menuBar()->addMenu(tr("&Action"));
    actionMenu->addAction(m_straightenAction);
    actionMenu->addAction(m_straightenMoreAction);

    // connect(openAction, &QAction::triggered, this, &MainWindow::openImage);
    // connect(saveAction, &QAction::triggered, this, &MainWindow::saveDocument);
    connect(quitAction, &QAction::triggered, this, &MainWindow::close);
    connect(m_straightenAction, &QAction::triggered, this, &MainWindow::straighten);
    connect(m_straightenMoreAction, &QAction::triggered, this, &MainWindow::straightenMore);
    connect(m_firstImage, &QAction::triggered, this, &MainWindow::firstImage);
    connect(m_lastImage, &QAction::triggered, this, &MainWindow::lastImage);
    connect(m_nextImage, &QAction::triggered, this, &MainWindow::nextImage);
    connect(m_previousImage, &QAction::triggered, this, &MainWindow::previousImage);

    setWindowTitle("Unbender");

    readSettings();
    updateFileList();
    firstImage();
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

void MainWindow::openImage(const QString &filePath)
{
    QImageReader reader(filePath);
    reader.setAutoTransform(true);
    QImage image = reader.read();

    if (!image.isNull())
    {
        QGraphicsPixmapItem *item = new QGraphicsPixmapItem(QPixmap::fromImage(image));
        m_processedView->clear();
        m_processedView->setImage(item);
        // m_processedView->fitInView(m_processedView->scene()->itemsBoundingRect(), Qt::KeepAspectRatio);
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

        updateUI();
    }
}

void MainWindow::saveDocument()
{
    updateUI();
}

void MainWindow::straighten()
{
    m_findCentreLine.setThresholdValue(m_sidebar->spinBox("threshold")->value());
    m_findCentreLine.setImg(OpenCVTools::convertQImageToMat(*m_image));

    MarkerItem *p1 = m_processedView->position1();
    m_findCentreLine.setUserPoint1(cv::Point2f(p1->pos().x(), p1->pos().y()));
    MarkerItem *p2 = m_processedView->position2();
    m_findCentreLine.setUserPoint2(cv::Point2f(p2->pos().x(), p2->pos().y()));

    m_findCentreLine.straighten();

    QImage thresholdImage = OpenCVTools::convertMatToQImage(m_findCentreLine.thresh());
    m_processedView->setImage(new QGraphicsPixmapItem(QPixmap::fromImage(thresholdImage)));

    QGraphicsPathItem *item;
    item = new QGraphicsPathItem(OpenCVTools::convertPolylineToQPainterPath(m_findCentreLine.polyA(), false));
    item->setPen(QPen(Qt::cyan, 2));
    item->setBrush(Qt::NoBrush);
    m_processedView->addExtraItem(item);
    item = new QGraphicsPathItem(OpenCVTools::convertPolylineToQPainterPath(m_findCentreLine.polyB(), false));
    item->setPen(QPen(Qt::magenta, 2));
    item->setBrush(Qt::NoBrush);
    m_processedView->addExtraItem(item);

    auto stickList = m_findCentreLine.stickList();
    for (size_t i = 0; i < stickList.size(); ++i)
    {
        item = new QGraphicsPathItem(OpenCVTools::convertPolylineToQPainterPath(stickList[i], false));
        item->setPen(QPen(Qt::darkYellow, 1));
        item->setBrush(Qt::NoBrush);
        m_processedView->addExtraItem(item);
    }

    item = new QGraphicsPathItem(OpenCVTools::convertPolylineToQPainterPath(m_findCentreLine.centreLine(), false));
    item->setPen(QPen(Qt::yellow, 2));
    item->setBrush(Qt::NoBrush);
    m_processedView->addExtraItem(item);

    updateUI();
}

void MainWindow::straightenMore()
{
    m_findCentreLine.straightenMore();

    m_processedView->clearExtrasItems();
    QGraphicsPathItem *item;
    item = new QGraphicsPathItem(OpenCVTools::convertPolylineToQPainterPath(m_findCentreLine.polyA(), false));
    item->setPen(QPen(Qt::cyan, 2));
    item->setBrush(Qt::NoBrush);
    m_processedView->addExtraItem(item);
    item = new QGraphicsPathItem(OpenCVTools::convertPolylineToQPainterPath(m_findCentreLine.polyB(), false));
    item->setPen(QPen(Qt::magenta, 2));
    item->setBrush(Qt::NoBrush);
    m_processedView->addExtraItem(item);

    auto stickList = m_findCentreLine.stickList();
    for (size_t i = 0; i < stickList.size(); ++i)
    {
        item = new QGraphicsPathItem(OpenCVTools::convertPolylineToQPainterPath(stickList[i], false));
        item->setPen(QPen(Qt::darkGreen, 1));
        item->setBrush(Qt::NoBrush);
        m_processedView->addExtraItem(item);
    }


    item = new QGraphicsPathItem(OpenCVTools::convertPolylineToQPainterPath(m_findCentreLine.centreLine(), false));
    item->setPen(QPen(Qt::green, 1));
    item->setBrush(Qt::NoBrush);
    m_processedView->addExtraItem(item);

    m_findCentreLine.createStraightVersion();
    auto mesh = m_findCentreLine.straightMesh();
    std::string objVersion = OpenCVTools::meshToOBJ(mesh, "straight_mesh");
    std::ofstream("C:\\Scratch\\output.obj") << objVersion;
    m_meshView->setMeshes({mesh});

    updateUI();
}


void MainWindow::updateUI()
{
    QFileInfo inputFolderInfo(m_sidebar->pathEditWidget("masksFolder")->path());
    QFileInfo outputFolderInfo(m_sidebar->pathEditWidget("outputFolder")->path());
    bool inputFolderValid = inputFolderInfo.isDir() && inputFolderInfo.isReadable();
    bool outputFolderValid = outputFolderInfo.isDir() && outputFolderInfo.isReadable() && outputFolderInfo.isWritable();
    m_straightenAction->setEnabled(inputFolderValid && outputFolderValid && m_image != 0 && m_processedView->position1() && m_processedView->position2());
    m_straightenMoreAction->setEnabled(inputFolderValid && outputFolderValid && m_image != 0 && m_processedView->position1() && m_processedView->position2() && m_findCentreLine.polyA().size() && m_findCentreLine.polyB().size() && m_findCentreLine.centreLine().size());
    m_firstImage->setEnabled(inputFolderValid && outputFolderValid && m_imageFileList.size() > 0 && m_imageFileListIndex > 0);
    m_previousImage->setEnabled(inputFolderValid && outputFolderValid && m_imageFileList.size() > 0 && m_imageFileListIndex > 0);
    m_nextImage->setEnabled(inputFolderValid && outputFolderValid && m_imageFileList.size() > 0 && m_imageFileListIndex < m_imageFileList.size() - 1);
    m_lastImage->setEnabled(inputFolderValid && outputFolderValid && m_imageFileList.size() > 0 && m_imageFileListIndex < m_imageFileList.size() - 1);
}

void MainWindow::readSettings()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "AnimalSimulationLaboratory", "Unbender");
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
    m_splitter->restoreState(settings.value("splitterState").toByteArray());
    m_sidebar->pathEditWidget("framesFolder")->setPath(settings.value("framesFolder", "").toString());
    m_sidebar->pathEditWidget("masksFolder")->setPath(settings.value("masksFolder", "").toString());
    m_sidebar->pathEditWidget("outputFolder")->setPath(settings.value("outputFolder", "").toString());
    m_sidebar->spinBox("threshold")->setValue(settings.value("threshold", "127").toInt());
    m_sidebar->checkBox("invert")->setChecked(settings.value("invert", "0").toBool());
}

void MainWindow::writeSettings()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "AnimalSimulationLaboratory", "Unbender");
    settings.setValue("framesFolder", m_sidebar->pathEditWidget("framesFolder")->path());
    settings.setValue("masksFolder", m_sidebar->pathEditWidget("masksFolder")->path());
    settings.setValue("outputFolder", m_sidebar->pathEditWidget("outputFolder")->path());
    settings.setValue("threshold", m_sidebar->spinBox("threshold")->value());
    settings.setValue("invert", m_sidebar->checkBox("invert")->isChecked());
    settings.setValue("splitterState", m_splitter->saveState());
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    settings.sync();
}

void MainWindow::updateFileList()
{
    m_imageFileList.clear();
    m_imageFileListIndex = -1;
    QFileInfo inputFolderInfo(m_sidebar->pathEditWidget("masksFolder")->path());
    bool inputFolderValid = inputFolderInfo.isDir() && inputFolderInfo.isReadable();
    if (!inputFolderValid) {  return; }

    QDir dir(inputFolderInfo.absoluteFilePath());
    QStringList allFiles = dir.entryList(QDir::Files);
    QRegularExpression re(m_imageFileMatchRegex, QRegularExpression::CaseInsensitiveOption);
    for (auto &&file : allFiles)
    {
        if (re.match(file).hasMatch())
        {
            m_imageFileList << dir.absoluteFilePath(file);
        }
    }
    firstImage();
}

void MainWindow::firstImage()
{
    if (m_imageFileList.size() == 0)
    {
        m_imageFileListIndex = -1;
        return;
    }
    m_imageFileListIndex = 0;
    openImage(m_imageFileList[m_imageFileListIndex]);
}

void MainWindow::lastImage()
{
    if (m_imageFileList.size() == 0)
    {
        m_imageFileListIndex = -1;
        return;
    }
    m_imageFileListIndex = m_imageFileList.size() - 1;
    openImage(m_imageFileList[m_imageFileListIndex]);
}

void MainWindow::nextImage()
{
    if (m_imageFileList.size() == 0)
    {
        m_imageFileListIndex = -1;
        return;
    }
    ++m_imageFileListIndex;
    if (m_imageFileListIndex >= m_imageFileList.size()) m_imageFileListIndex = m_imageFileList.size() - 1;
    openImage(m_imageFileList[m_imageFileListIndex]);
}

void MainWindow::previousImage()
{
    if (m_imageFileList.size() == 0)
    {
        m_imageFileListIndex = -1;
        return;
    }
    --m_imageFileListIndex;
    if (m_imageFileListIndex < 0) m_imageFileListIndex = 0;
    openImage(m_imageFileList[m_imageFileListIndex]);
}
