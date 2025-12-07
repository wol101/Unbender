#include "MainWindow.h"
#include "./ui_MainWindow.h"

#include "GraphicsView.h"

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


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_ui(new Ui::MainWindow)
{
    m_ui->setupUi(this);

    m_scene = new QGraphicsScene(this);
    m_view = new GraphicsView(m_scene, this);
    setCentralWidget(m_view);

    connect(m_ui->actionOpen, &QAction::triggered, this, &MainWindow::openImage);
    connect(m_ui->actionStraighten, &QAction::triggered, this, &MainWindow::straighten);
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
            m_scene->clear();
            m_scene->addPixmap(QPixmap::fromImage(image));
            m_view->clear();
            m_view->fitInView(m_scene->itemsBoundingRect(), Qt::KeepAspectRatio);
            QImage grayImage = image.convertToFormat(QImage::Format_Grayscale8);
            m_image = std::make_unique<Image<uint8_t>>(image.width(), image.height());
            for (size_t y = 0; y < grayImage.height(); ++y)
            {
                uchar* row = grayImage.scanLine(y);  // raw pointer to row y
                // Interpret row depending on format, e.g. ARGB32
                // QRgb* pixels = reinterpret_cast<QRgb*>(row); // not needed for Format_Grayscale8
                m_image->setRow(row, y);
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
    updateUI();
}

void MainWindow::updateUI()
{
    m_ui->actionOpen->setEnabled(true);
    m_ui->actionStraighten->setEnabled(m_view->position1() && m_view->position2());
}

void MainWindow::readSettings()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "AnimalSimulationLaboratory", "Unbender");
    m_filePath = settings.value("lastFileOpened", "").toString();
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
}

void MainWindow::writeSettings()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "AnimalSimulationLaboratory", "Unbender");
    settings.setValue("lastFileOpened", m_filePath);
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    settings.sync();
}

