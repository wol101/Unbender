#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "Image.h"

class QGraphicsScene;
class GraphicsView;

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

private slots:
    void openImage();

private:
    Ui::MainWindow *m_ui = 0;

    QGraphicsScene *m_scene = 0;
    GraphicsView *m_view = 0;
    QString m_filePath;

    std::unique_ptr<Image<uint8_t>> m_image;


    void readSettings();
    void writeSettings();

};
#endif // MAINWINDOW_H
