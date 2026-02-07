#ifndef PATHEDITWIDGET_H
#define PATHEDITWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>

class PathEditWidget : public QWidget
{
    Q_OBJECT

public:
    enum Mode {
        FileMode,
        DirectoryMode
    };

    explicit PathEditWidget(Mode mode, const QString& buttonText = "Browse...", QWidget* parent = nullptr);

    QString path() const;
    void setPath(const QString& p);

signals:
    void pathChanged(const QString& newPath);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private slots:
    void browse();
    void validatePath();

private:
    void updateStyle(bool valid);

    Mode m_mode;
    QLineEdit* m_edit;
    QPushButton* m_button;

    bool m_valid = false;
};

#endif // PATHEDITWIDGET_H
