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
#include <QLineEdit>
#include <QWidgetAction>
#include <QSpinBox>

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
    // auto *saveAction = new QAction(style()->standardIcon(QStyle::SP_DialogSaveButton), tr("Save"), this);
    auto *quitAction = new QAction(style()->standardIcon(QStyle::SP_TitleBarCloseButton), tr("Quit"), this);

    m_inputFolderAction = new QAction(style()->standardIcon(QStyle::SP_DialogOpenButton), tr("Input Folder..."), this);
    m_outputFolderAction = new QAction(style()->standardIcon(QStyle::SP_DialogOpenButton), tr("Output Folder..."), this);

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

    toolbar->addSeparator();

    m_spinBoxThreshold = new QSpinBox(this);
    m_spinBoxThreshold->setMinimum(0);
    m_spinBoxThreshold->setMaximum(255);
    widgetAction = new QWidgetAction(this);
    widgetAction->setDefaultWidget(m_spinBoxThreshold);
    toolbar->addAction(widgetAction);

    toolbar->addAction(m_inputFolderAction);
    m_lineEditInputFolder = new QLineEdit(this);
    m_lineEditInputFolder->setPlaceholderText("Input Folder...");
    widgetAction = new QWidgetAction(this);
    widgetAction->setDefaultWidget(m_lineEditInputFolder);
    toolbar->addAction(widgetAction);

    toolbar->addSeparator();

    toolbar->addAction(m_outputFolderAction);
    m_lineEditOutputFolder = new QLineEdit(this);
    m_lineEditOutputFolder->setPlaceholderText("Output Folder...");
    widgetAction = new QWidgetAction(this);
    widgetAction->setDefaultWidget(m_lineEditOutputFolder);
    toolbar->addAction(widgetAction);

    // Create menus
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(m_inputFolderAction);
    fileMenu->addAction(m_outputFolderAction);
    fileMenu->addSeparator();
    fileMenu->addSeparator();
    fileMenu->addAction(quitAction);

    QMenu *actionMenu = menuBar()->addMenu(tr("&Action"));
    actionMenu->addAction(m_straightenAction);
    actionMenu->addAction(m_straightenMoreAction);

    // connect(openAction, &QAction::triggered, this, &MainWindow::openImage);
    // connect(saveAction, &QAction::triggered, this, &MainWindow::saveDocument);
    connect(quitAction, &QAction::triggered, this, &MainWindow::close);
    connect(m_straightenAction, &QAction::triggered, this, &MainWindow::straighten);
    connect(m_straightenMoreAction, &QAction::triggered, this, &MainWindow::straightenMore);
    connect(m_inputFolderAction, &QAction::triggered, this, &MainWindow::inputFolder);
    connect(m_outputFolderAction, &QAction::triggered, this, &MainWindow::outputFolder);
    connect(m_firstImage, &QAction::triggered, this, &MainWindow::firstImage);
    connect(m_lastImage, &QAction::triggered, this, &MainWindow::lastImage);
    connect(m_nextImage, &QAction::triggered, this, &MainWindow::nextImage);
    connect(m_previousImage, &QAction::triggered, this, &MainWindow::previousImage);
    connect(m_lineEditInputFolder, &QLineEdit::editingFinished, this, &MainWindow::updateFileList);

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
        m_view->clear();
        m_view->setImage(item);
        // m_view->fitInView(m_view->scene()->itemsBoundingRect(), Qt::KeepAspectRatio);
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
    m_findCentreLine.setThresholdValue(m_spinBoxThreshold->value());
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
    QFileInfo inputFolderInfo(m_lineEditInputFolder->text());
    QFileInfo outputFolderInfo(m_lineEditOutputFolder->text());
    bool inputFolderValid = inputFolderInfo.isDir() && inputFolderInfo.isReadable();
    bool outputFolderValid = outputFolderInfo.isDir() && outputFolderInfo.isReadable() && outputFolderInfo.isWritable();
    m_inputFolderAction->setEnabled(true);
    m_outputFolderAction->setEnabled(true);
    m_straightenAction->setEnabled(inputFolderValid && outputFolderValid && m_image != 0 && m_view->position1() && m_view->position2());
    m_straightenMoreAction->setEnabled(inputFolderValid && outputFolderValid && m_image != 0 && m_view->position1() && m_view->position2() && m_findCentreLine.polyA().size() && m_findCentreLine.polyB().size() && m_findCentreLine.centreLine().size());
    m_firstImage->setEnabled(inputFolderValid && outputFolderValid && m_imageFileList.size() > 0 && m_imageFileListIndex > 0);
    m_previousImage->setEnabled(inputFolderValid && outputFolderValid && m_imageFileList.size() > 0 && m_imageFileListIndex > 0);
    m_nextImage->setEnabled(inputFolderValid && outputFolderValid && m_imageFileList.size() > 0 && m_imageFileListIndex < m_imageFileList.size() - 1);
    m_lastImage->setEnabled(inputFolderValid && outputFolderValid && m_imageFileList.size() > 0 && m_imageFileListIndex < m_imageFileList.size() - 1);
}

void MainWindow::readSettings()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "AnimalSimulationLaboratory", "Unbender");
    m_lineEditInputFolder->setText(settings.value("inputFolder", "").toString());
    m_lineEditOutputFolder->setText(settings.value("outputFolder", "").toString());
    m_spinBoxThreshold->setValue(settings.value("threshold", "").toInt());
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
    m_splitter->restoreState(settings.value("splitterState").toByteArray());
}

void MainWindow::writeSettings()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "AnimalSimulationLaboratory", "Unbender");
    settings.setValue("inputFolder", m_lineEditInputFolder->text());
    settings.setValue("outputFolder", m_lineEditOutputFolder->text());
    settings.setValue("threshold", m_spinBoxThreshold->value());
    settings.setValue("splitterState", m_splitter->saveState());
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    settings.sync();
}

void MainWindow::inputFolder()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Input Folder", m_lineEditInputFolder->text());

    if (!dir.isEmpty())
    {
        m_lineEditInputFolder->setText(dir);
        updateFileList();
    }
    updateUI();
}

void MainWindow::outputFolder()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Output Folder", m_lineEditOutputFolder->text());

    if (!dir.isEmpty())
    {
        m_lineEditOutputFolder->setText(dir);
    }
    updateUI();
}

void MainWindow::updateFileList()
{
    m_imageFileList.clear();
    m_imageFileListIndex = -1;
    QFileInfo inputFolderInfo(m_lineEditInputFolder->text());
    bool inputFolderValid = inputFolderInfo.isDir() && inputFolderInfo.isReadable();
    if (!inputFolderValid) {  return; }

    QDir dir(m_lineEditInputFolder->text());
    QStringList allFiles = dir.entryList(QDir::Files);
    QRegularExpression re(m_imageFileMatchRegex, QRegularExpression::CaseInsensitiveOption);
    for (auto &&file : allFiles)
    {
        if (re.match(file).hasMatch())
        {
            m_imageFileList << dir.absoluteFilePath(file);
        }
    }
    updateUI();
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
