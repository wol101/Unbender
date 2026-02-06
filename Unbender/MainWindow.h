#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "FindCentreLine.h"

#include <QMainWindow>

class GraphicsView;
class MeshViewWidget;
class QGraphicsScene;
class QImage;
class QSplitter;
class QLineEdit;
class QSpinBox;

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
    void inputFolder();
    void outputFolder();
    void updateFileList();
    void firstImage();
    void lastImage();
    void nextImage();
    void previousImage();

private:
    Ui::MainWindow *m_ui = 0;

    GraphicsView *m_view = 0;
    MeshViewWidget* m_meshView = 0;
    QSplitter* m_splitter = 0;
    QLineEdit *m_lineEditInputFolder = 0;
    QLineEdit *m_lineEditOutputFolder = 0;
    QSpinBox *m_spinBoxThreshold = 0;

    QAction *m_inputFolderAction = 0;
    QAction *m_outputFolderAction = 0;
    QAction *m_straightenAction = 0;
    QAction *m_straightenMoreAction = 0;
    QAction *m_firstImage = 0;
    QAction *m_lastImage = 0;
    QAction *m_nextImage = 0;
    QAction *m_previousImage = 0;

    QStringList m_imageFileList;
    int m_imageFileListIndex = -1;
    QString m_imageFileMatchRegex = "mask.*\\.png";
    std::unique_ptr<QImage> m_image;
    bool m_unsavedChanges = false;

    void readSettings();
    void writeSettings();
    void openImage(const QString &filePath);


    FindCentreLine m_findCentreLine;

};
#endif // MAINWINDOW_H
