#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "FindCentreLine.h"

#include <QMainWindow>

class GraphicsView;
class MeshViewWidget;
class Sidebar;
class FourPaneViewport;

class QGraphicsScene;
class QImage;
class QSplitter;
class QLineEdit;
class QSpinBox;
class QCheckBox;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void updateUI();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void saveDocument();
    void straighten();
    void straightenMore();
    void updateFileList();
    void firstImage();
    void lastImage();
    void nextImage();
    void previousImage();

private:
    Ui::MainWindow *m_ui = 0;

    Sidebar *m_sidebar;
    FourPaneViewport *m_fourPaneViewport;

    GraphicsView *m_frameView = 0;
    GraphicsView *m_maskView = 0;
    GraphicsView *m_processedView = 0;
    MeshViewWidget* m_meshView = 0;
    QSplitter* m_splitter = 0;

    QAction *m_straightenAction = 0;
    QAction *m_straightenMoreAction = 0;
    QAction *m_firstImage = 0;
    QAction *m_lastImage = 0;
    QAction *m_nextImage = 0;
    QAction *m_previousImage = 0;

    struct ImageSet
    {
        QImage frameImage;
        QImage maskImage;
        QImage outputImage;
        OpenCVTools::Mesh outputMesh;
        QString frameImagePath;
        QString maskImagePath;
        QString outputImagePath;
        QString outputMeshPath;
    };
    QList<std::unique_ptr<ImageSet>> m_imageSetList;
    int m_imageFileListIndex = -1;
    bool m_unsavedChanges = false;
    bool m_framesFolderValid = false;
    bool m_masksFolderValid = false;
    bool m_outputImageFolderValid = false;
    bool m_outputMeshFolderValid = false;

    void readSettings();
    void writeSettings();
    void openImage();
    void thresholdImage();
    void createMesh();
    static QImage readImageEndednessIndependent(const QString &imagePath);

    FindCentreLine m_findCentreLine;

};
#endif // MAINWINDOW_H
