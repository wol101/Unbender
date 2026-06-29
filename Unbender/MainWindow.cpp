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
    m_sidebar->addPathEditWidget("Output Images", "outputImageFolder", PathEditWidget::DirectoryMode, "");
    m_sidebar->addPathEditWidget("Output Meshes", "outputMeshFolder", PathEditWidget::DirectoryMode, "");
    m_sidebar->addSpinBox("Threshold", "threshold", 0, 255, 127);
    m_sidebar->addCheckBox("Invert", "invert", 0);
    m_sidebar->addSpacer();

    // Right side: the four pane viewport

    // 3 of the views are image viewers
    QGraphicsScene *scene;
    scene = new QGraphicsScene(this);
    m_frameView = new GraphicsView(scene);
    scene = new QGraphicsScene(this);
    m_maskView = new GraphicsView(scene);
    scene = new QGraphicsScene(this);
    m_processedView = new GraphicsView(scene);

    // 1 view is the 3D mesh viewer
    m_meshView = new MeshViewWidget(this);

    m_fourPaneViewport = new FourPaneViewport(m_frameView, m_maskView, m_processedView, m_meshView, this);

    // Add widgets to splitter
    m_splitter->addWidget(m_sidebar);
    m_splitter->addWidget(m_fourPaneViewport);

    // Force even split
    m_splitter->setSizes({1, 4});

    // Create actions
    auto *quitAction = new QAction(style()->standardIcon(QStyle::SP_TitleBarCloseButton), tr("Quit"), this);

    m_straightenAction = new QAction(tr("Straighten"), this);
    m_straightenMoreAction = new QAction(tr("Straighten More"), this);
    m_createMeshAction = new QAction(tr("Create Mesh"), this);

    m_firstImage = new QAction(style()->standardIcon(QStyle::SP_MediaSkipBackward), tr("First Image"), this);
    m_lastImage = new QAction(style()->standardIcon(QStyle::SP_MediaSkipForward), tr("Last Images"), this);
    m_nextImage = new QAction(style()->standardIcon(QStyle::SP_MediaSeekForward), tr("Next Image"), this);
    m_previousImage = new QAction(style()->standardIcon(QStyle::SP_MediaSeekBackward), tr("Previous Images"), this);

    m_nextImage->setShortcut(Qt::ALT | Qt::Key_Right);
    m_previousImage->setShortcut(Qt::ALT | Qt::Key_Left);

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
    actionMenu->addAction(m_createMeshAction);

    connect(quitAction, &QAction::triggered, this, &MainWindow::close);
    connect(m_straightenAction, &QAction::triggered, this, &MainWindow::straighten);
    connect(m_straightenMoreAction, &QAction::triggered, this, &MainWindow::straightenMore);
    connect(m_createMeshAction, &QAction::triggered, this, &MainWindow::createMesh);
    connect(m_firstImage, &QAction::triggered, this, &MainWindow::firstImage);
    connect(m_lastImage, &QAction::triggered, this, &MainWindow::lastImage);
    connect(m_nextImage, &QAction::triggered, this, &MainWindow::nextImage);
    connect(m_previousImage, &QAction::triggered, this, &MainWindow::previousImage);
    connect(m_maskView, &GraphicsView::uiUpdateRequested, this, &MainWindow::updateUI);

    setWindowTitle("Unbender");

    readSettings();
    updateUI();
    updateFileList();
    firstImage();
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

void MainWindow::saveDocument()
{
    updateUI();
}

void MainWindow::straighten()
{
    ImageSet *imageSet = m_imageSetList[m_imageSetListIndex].get();
    m_findCentreLine = std::make_unique<FindCentreLine>();
    m_findCentreLine->setThresholdValue(m_sidebar->spinBox("threshold")->value());
    m_findCentreLine->setImg(OpenCVTools::convertQImageToMat(imageSet->frameImage));

    MarkerItem *p1 = m_maskView->position1();
    MarkerItem *p2 = m_maskView->position2();
    if (!p1 || !p2) return;
    imageSet->pStart = cv::Point2f(p1->pos().x(), p1->pos().y());
    imageSet->pEnd = cv::Point2f(p2->pos().x(), p2->pos().y());
    m_findCentreLine->setUserPoint1(imageSet->pStart);
    m_findCentreLine->setUserPoint2(imageSet->pEnd);

    m_findCentreLine->straighten();

    QImage thresholdImage = OpenCVTools::convertMatToQImage(m_findCentreLine->thresh());
    m_processedView->setImage(new QGraphicsPixmapItem(QPixmap::fromImage(thresholdImage)));

    QGraphicsPathItem *item;
    item = new QGraphicsPathItem(OpenCVTools::convertPolylineToQPainterPath(m_findCentreLine->polyA(), false));
    item->setPen(QPen(Qt::cyan, 2));
    item->setBrush(Qt::NoBrush);
    m_processedView->addExtraItem(item);
    item = new QGraphicsPathItem(OpenCVTools::convertPolylineToQPainterPath(m_findCentreLine->polyB(), false));
    item->setPen(QPen(Qt::magenta, 2));
    item->setBrush(Qt::NoBrush);
    m_processedView->addExtraItem(item);

    auto stickList = m_findCentreLine->stickList();
    for (size_t i = 0; i < stickList.size(); ++i)
    {
        item = new QGraphicsPathItem(OpenCVTools::convertPolylineToQPainterPath(stickList[i], false));
        item->setPen(QPen(Qt::darkYellow, 1));
        item->setBrush(Qt::NoBrush);
        m_processedView->addExtraItem(item);
    }

    item = new QGraphicsPathItem(OpenCVTools::convertPolylineToQPainterPath(m_findCentreLine->centreLine(), false));
    item->setPen(QPen(Qt::yellow, 2));
    item->setBrush(Qt::NoBrush);
    m_processedView->addExtraItem(item);

    QDir outputImageFolder(m_sidebar->pathEditWidget("outputImageFolder")->path());
    QFileInfo fileInfo(imageSet->maskImagePath);
    imageSet->outputImagePath = outputImageFolder.absoluteFilePath(replaceExtension(fileInfo.fileName(), ".png"));
    imageSet->outputImage = m_processedView->renderSceneToImage();
    imageSet->outputImage.save(imageSet->outputImagePath);

    // mesh is no longer valid
    QDir outputMeshFolder(m_sidebar->pathEditWidget("outputMeshFolder")->path());
    imageSet->outputMeshPath = outputMeshFolder.absoluteFilePath(replaceExtension(fileInfo.fileName(), ".obj"));
    QFile::remove(imageSet->outputMeshPath);
    imageSet->outputMeshPath = "";
    imageSet->outputMesh = OpenCVTools::Mesh();
    m_meshView->setMeshes({imageSet->outputMesh});

    imageSet->findCentreLine = *m_findCentreLine;
    updateUI();
}

void MainWindow::straightenMore()
{
    ImageSet *imageSet = m_imageSetList[m_imageSetListIndex].get();
    if (!m_findCentreLine) return;
    m_findCentreLine->straightenMore();

    m_processedView->clearExtrasItems();
    QGraphicsPathItem *item;
    item = new QGraphicsPathItem(OpenCVTools::convertPolylineToQPainterPath(m_findCentreLine->polyA(), false));
    item->setPen(QPen(Qt::cyan, 2));
    item->setBrush(Qt::NoBrush);
    m_processedView->addExtraItem(item);
    item = new QGraphicsPathItem(OpenCVTools::convertPolylineToQPainterPath(m_findCentreLine->polyB(), false));
    item->setPen(QPen(Qt::magenta, 2));
    item->setBrush(Qt::NoBrush);
    m_processedView->addExtraItem(item);

    auto stickList = m_findCentreLine->stickList();
    for (size_t i = 0; i < stickList.size(); ++i)
    {
        item = new QGraphicsPathItem(OpenCVTools::convertPolylineToQPainterPath(stickList[i], false));
        item->setPen(QPen(Qt::darkGreen, 1));
        item->setBrush(Qt::NoBrush);
        m_processedView->addExtraItem(item);
    }


    item = new QGraphicsPathItem(OpenCVTools::convertPolylineToQPainterPath(m_findCentreLine->centreLine(), false));
    item->setPen(QPen(Qt::green, 1));
    item->setBrush(Qt::NoBrush);
    m_processedView->addExtraItem(item);

    m_findCentreLine->createStraightVersion();
    imageSet->findCentreLine = *m_findCentreLine;
    updateUI();
}

void MainWindow::createMesh()
{
    ImageSet *imageSet = m_imageSetList[m_imageSetListIndex].get();
    QFileInfo fileInfo(imageSet->maskImagePath);
    QDir outputMeshFolder(m_sidebar->pathEditWidget("outputMeshFolder")->path());
    imageSet->outputMeshPath = outputMeshFolder.absoluteFilePath(replaceExtension(fileInfo.fileName(), ".obj"));
    auto mesh = m_findCentreLine->straightMesh();
    std::string objVersion = OpenCVTools::meshToOBJ(mesh, "straight_mesh");
    std::ofstream(imageSet->outputMeshPath.toStdString()) << objVersion;
    m_meshView->setMeshes({mesh});
    updateUI();
}

void MainWindow::updateUI()
{
    const ImageSet *imageSet;
    const static ImageSet nullImageSet;
    if (m_imageSetListIndex >= 0) imageSet = m_imageSetList[m_imageSetListIndex].get();
    else imageSet = &nullImageSet;
    QFileInfo framesFolderInfo(m_sidebar->pathEditWidget("framesFolder")->path());
    QFileInfo masksFolderInfo(m_sidebar->pathEditWidget("masksFolder")->path());
    QFileInfo outputImageFolderInfo(m_sidebar->pathEditWidget("outputImageFolder")->path());
    QFileInfo outputMeshFolderInfo(m_sidebar->pathEditWidget("outputMeshFolder")->path());
    m_framesFolderValid = framesFolderInfo.isDir() && framesFolderInfo.isReadable();
    m_masksFolderValid = masksFolderInfo.isDir() && masksFolderInfo.isReadable() && masksFolderInfo.isWritable();
    m_outputImageFolderValid = outputImageFolderInfo.isDir() && outputImageFolderInfo.isReadable() && outputImageFolderInfo.isWritable();
    m_outputMeshFolderValid = outputMeshFolderInfo.isDir() && outputMeshFolderInfo.isReadable() && outputMeshFolderInfo.isWritable();
    m_straightenAction->setEnabled(m_framesFolderValid && m_masksFolderValid && m_outputImageFolderValid && m_outputMeshFolderValid &&
                                   !imageSet->maskImage.isNull() &&  m_maskView->position1() && m_maskView->position2());
    m_straightenMoreAction->setEnabled(m_framesFolderValid && m_masksFolderValid && m_outputImageFolderValid && m_outputMeshFolderValid &&
                                       !imageSet->maskImage.isNull() && !imageSet->outputImage.isNull() && m_findCentreLine &&
                                       m_maskView->position1() && m_maskView->position2());
    m_createMeshAction->setEnabled(m_framesFolderValid && m_masksFolderValid && m_outputImageFolderValid && m_outputMeshFolderValid &&
                                       !imageSet->maskImage.isNull() && !imageSet->outputImage.isNull() && m_findCentreLine &&
                                       m_maskView->position1() && m_maskView->position2() && m_findCentreLine->straightMesh().triangles.size());
    m_firstImage->setEnabled(m_framesFolderValid && m_masksFolderValid && m_outputImageFolderValid && m_outputMeshFolderValid &&
                             m_imageSetList.size() > 0 && m_imageSetListIndex > 0);
    m_previousImage->setEnabled(m_framesFolderValid && m_masksFolderValid && m_outputImageFolderValid && m_outputMeshFolderValid &&
                                m_imageSetList.size() > 0 && m_imageSetListIndex > 0);
    m_nextImage->setEnabled(m_framesFolderValid && m_masksFolderValid && m_outputImageFolderValid && m_outputMeshFolderValid &&
                            m_imageSetList.size() > 0 && m_imageSetListIndex < m_imageSetList.size() - 1);
    m_lastImage->setEnabled(m_framesFolderValid && m_masksFolderValid && m_outputImageFolderValid && m_outputMeshFolderValid &&
                            m_imageSetList.size() > 0 && m_imageSetListIndex < m_imageSetList.size() - 1);
}

void MainWindow::readSettings()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "AnimalSimulationLaboratory", "Unbender");
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
    m_splitter->restoreState(settings.value("splitterState").toByteArray());
    m_sidebar->pathEditWidget("framesFolder")->setPath(settings.value("framesFolder", "").toString());
    m_sidebar->pathEditWidget("masksFolder")->setPath(settings.value("masksFolder", "").toString());
    m_sidebar->pathEditWidget("outputImageFolder")->setPath(settings.value("outputImageFolder", "").toString());
    m_sidebar->pathEditWidget("outputMeshFolder")->setPath(settings.value("outputMeshFolder", "").toString());
    m_sidebar->spinBox("threshold")->setValue(settings.value("threshold", "127").toInt());
    m_sidebar->checkBox("invert")->setChecked(settings.value("invert", "0").toBool());
}

void MainWindow::writeSettings()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "AnimalSimulationLaboratory", "Unbender");
    settings.setValue("framesFolder", m_sidebar->pathEditWidget("framesFolder")->path());
    settings.setValue("masksFolder", m_sidebar->pathEditWidget("masksFolder")->path());
    settings.setValue("outputImageFolder", m_sidebar->pathEditWidget("outputImageFolder")->path());
    settings.setValue("outputMeshFolder", m_sidebar->pathEditWidget("outputMeshFolder")->path());
    settings.setValue("threshold", m_sidebar->spinBox("threshold")->value());
    settings.setValue("invert", m_sidebar->checkBox("invert")->isChecked());
    settings.setValue("splitterState", m_splitter->saveState());
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    settings.sync();
}

void MainWindow::updateFileList()
{
    m_imageSetList.clear();
    m_imageSetListIndex = -1;
    QFileInfo framesFolderInfo(m_sidebar->pathEditWidget("framesFolder")->path());
    QFileInfo masksFolderInfo(m_sidebar->pathEditWidget("masksFolder")->path());
    QFileInfo outputImageFolderInfo(m_sidebar->pathEditWidget("outputImageFolder")->path());
    QFileInfo outputMeshFolderInfo(m_sidebar->pathEditWidget("outputMeshFolder")->path());
    m_framesFolderValid = framesFolderInfo.isDir() && framesFolderInfo.isReadable();
    m_masksFolderValid = masksFolderInfo.isDir() && masksFolderInfo.isReadable() && masksFolderInfo.isWritable();
    m_outputImageFolderValid = outputImageFolderInfo.isDir() && outputImageFolderInfo.isReadable() && outputImageFolderInfo.isWritable();
    m_outputMeshFolderValid = outputMeshFolderInfo.isDir() && outputMeshFolderInfo.isReadable() && outputMeshFolderInfo.isWritable();
    if (!m_framesFolderValid || !m_masksFolderValid || !m_outputImageFolderValid || !m_outputMeshFolderValid) return;

    // get the image formats that Qt supports dynamically (because it depends on plug ins)
    QStringList fmts;
    for (const QByteArray &fmt : QImageReader::supportedImageFormats()) { fmts << QRegularExpression::escape(QString::fromLatin1(fmt)); }
    QRegularExpression imageRegex(QString(R"(.*\.(%1)$)").arg(fmts.join("|")), QRegularExpression::CaseInsensitiveOption );

    QDir framesFolder(m_sidebar->pathEditWidget("framesFolder")->path());
    QDir masksFolder(m_sidebar->pathEditWidget("masksFolder")->path());
    QDir outputImageFolder(m_sidebar->pathEditWidget("outputImageFolder")->path());
    QDir outputMeshFolder(m_sidebar->pathEditWidget("outputMeshFolder")->path());
    QStringList allFiles = masksFolder.entryList(QDir::Files);
    for (auto &&file : allFiles)
    {
        if (imageRegex.match(file).hasMatch()) // only image files
        {
            std::unique_ptr<ImageSet> imageSet = std::make_unique<ImageSet>();
            // there must always be a mask
            QString path = masksFolder.absoluteFilePath(file);
            QImage image = readImageEndednessIndependent(path);
            if (image.isNull()) continue;
            imageSet->maskImagePath = path;
            imageSet->maskImage = image;
            // but the others might not exist yet
            path = framesFolder.absoluteFilePath(file);
            image = readImageEndednessIndependent(path);
            if (!image.isNull())
            {
                imageSet->frameImagePath = path;
                imageSet->frameImage = image;
            }
            path = outputImageFolder.absoluteFilePath(file);
            image = readImageEndednessIndependent(path);
            if (!image.isNull())
            {
                imageSet->outputImagePath = path;
                imageSet->outputImage = image;
            }
            path = outputMeshFolder.absoluteFilePath(replaceExtension(file, ".obj"));
            OpenCVTools::Mesh mesh;
            OpenCVTools::ObjError err = OpenCVTools::loadObj(path.toStdString(), mesh);
            if (err == OpenCVTools::ObjError::OK)
            {
                imageSet->outputMeshPath = path;
                imageSet->outputMesh = mesh;
            }
            m_imageSetList.push_back(std::move(imageSet));
        }
    }
}

QImage MainWindow::readImageEndednessIndependent(const QString &imagePath)
{
    QImageReader reader(imagePath);
    reader.setAutoTransform(true);
    QImage image = reader.read();

    if (!image.isNull())
    {
        // I want to stadardise on the endedness independent QImage formats
        switch (image.format())
        {
        // these formats are used unchanged because they are endian indepenndent
        case QImage::Format_Grayscale8:
        case QImage::Format_RGB888:
        case QImage::Format_RGBA8888:
            return image;
            break;
        // these formats need to be converted
        case QImage::QImage::Format_Grayscale16:
            return image.convertToFormat(QImage::Format_Grayscale8);
            break;
        case QImage::QImage::Format_RGB32:
            return image.convertToFormat(QImage::Format_RGB888);
            break;
        case QImage::QImage::Format_ARGB32:
            return image.convertToFormat(QImage::Format_RGBA8888);
            break;
        // everything else to Format_RGB888
        default:
            return image.convertToFormat(QImage::Format_RGB888);
            break;
        }
    }
    return image;
}

void MainWindow::processCurrentImage()
{
    ImageSet *imageSet = m_imageSetList[m_imageSetListIndex].get();
    if (!imageSet)
    {
        qDebug() << "Warning: null image set in MainWindow::processCurrentImage()";
        return;
    }
    // simply display the original frame if it exists
    if (!imageSet->frameImage.isNull()) { m_frameView->setImage(new QGraphicsPixmapItem(QPixmap::fromImage(imageSet->frameImage))); }
    // there has to be a mask view to process so check here
    if (!imageSet->maskImage.isNull())
    {
        setWindowTitle(QString("Unbender: ") + imageSet->maskImagePath);
        m_maskView->setImage(new QGraphicsPixmapItem(QPixmap::fromImage(imageSet->maskImage)));
    }
    else
    {
        qDebug() << "Warning: maskImage missing in MainWindow::processCurrentImage()";
        return;
    }
    if (imageSet->pStart != cv::Point2f{-1.f, -1.f}) m_frameView->setPosition1(QPointF(imageSet->pStart.x, imageSet->pStart.x));
    if (imageSet->pEnd != cv::Point2f{-1.f, -1.f}) m_frameView->setPosition2(QPointF(imageSet->pEnd.x, imageSet->pEnd.x));
    // has the outputImage already been calculated
    if (!imageSet->outputImage.isNull()) { m_processedView->setImage(new QGraphicsPixmapItem(QPixmap::fromImage(imageSet->outputImage))); }
    else
    {
        if (m_straightenAction->isEnabled())
        {
            straighten();
            for (size_t i = 0; i < m_straightenMoreCount; ++i) straightenMore();
            QDir outputImageFolder(m_sidebar->pathEditWidget("outputImageFolder")->path());
            QFileInfo fileInfo(imageSet->maskImagePath);
            imageSet->outputImagePath = outputImageFolder.absoluteFilePath(replaceExtension(fileInfo.fileName(), ".png"));
            imageSet->outputImage = m_processedView->renderSceneToImage();
            imageSet->outputImage.save(imageSet->outputImagePath);

            QDir outputMeshFolder(m_sidebar->pathEditWidget("outputMeshFolder")->path());
            imageSet->outputMeshPath = outputMeshFolder.absoluteFilePath(replaceExtension(fileInfo.fileName(), ".obj"));
            auto mesh = m_findCentreLine->straightMesh();
            std::string objVersion = OpenCVTools::meshToOBJ(mesh, "straight_mesh");
            std::ofstream(imageSet->outputMeshPath.toStdString()) << objVersion;
            m_meshView->setMeshes({mesh});
        }
    }
}

void MainWindow::firstImage()
{
    if (m_imageSetList.size() == 0)
    {
        m_imageSetListIndex = -1;
        return;
    }
    m_imageSetListIndex = 0;
    processCurrentImage();
}

void MainWindow::lastImage()
{
    if (m_imageSetList.size() == 0)
    {
        m_imageSetListIndex = -1;
        return;
    }
    m_imageSetListIndex = m_imageSetList.size() - 1;
    processCurrentImage();
}

void MainWindow::nextImage()
{
    if (m_imageSetList.size() == 0)
    {
        m_imageSetListIndex = -1;
        return;
    }
    ++m_imageSetListIndex;
    if (m_imageSetListIndex >= m_imageSetList.size()) m_imageSetListIndex = m_imageSetList.size() - 1;
    processCurrentImage();
}

void MainWindow::previousImage()
{
    if (m_imageSetList.size() == 0)
    {
        m_imageSetListIndex = -1;
        return;
    }
    --m_imageSetListIndex;
    if (m_imageSetListIndex < 0) m_imageSetListIndex = 0;
    processCurrentImage();
}

QString MainWindow::replaceExtension(const QString &filePath, const QString &newExt)
{
    QFileInfo fi(filePath);
    return fi.path() + "/" + fi.completeBaseName() + "." + newExt;
}
