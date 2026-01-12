#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "OpenCVTools.h"

#include <QMainWindow>

class QGraphicsScene;
class GraphicsView;
class QImage;
class QSplitter;

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
    void openImage();
    void straighten();
    void straightenMore();
    void saveDocument();

private:
    Ui::MainWindow *m_ui = 0;

    GraphicsView *m_view = 0;
    QString m_filePath;
    QSplitter* m_splitter = 0;
    std::unique_ptr<QImage> m_image;
    bool m_unsavedChanges = false;

    void readSettings();
    void writeSettings();

    std::vector<cv::Point2f> m_polyA;
    std::vector<cv::Point2f> m_polyB;
    std::vector<cv::Point2f> m_centreLine;


};
#endif // MAINWINDOW_H
