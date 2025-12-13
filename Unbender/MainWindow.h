#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QGraphicsScene;
class GraphicsView;
class QImage;

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
    void saveDocument();

private:
    Ui::MainWindow *m_ui = 0;

    GraphicsView *m_view = 0;
    QString m_filePath;

    std::unique_ptr<QImage> m_image;
    bool m_unsavedChanges = false;

    void readSettings();
    void writeSettings();

};
#endif // MAINWINDOW_H
