#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "FindCentreLine.h"

#include <QMainWindow>

class GraphicsView;
class MeshViewWidget;
class QGraphicsScene;
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
    MeshViewWidget* m_meshView = 0;
    QString m_filePath;
    QSplitter* m_splitter = 0;
    std::unique_ptr<QImage> m_image;
    bool m_unsavedChanges = false;

    void readSettings();
    void writeSettings();

    FindCentreLine m_findCentreLine;

};
#endif // MAINWINDOW_H
