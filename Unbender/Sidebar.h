#ifndef SIDEBAR_H
#define SIDEBAR_H

#include <QWidget>
#include <QGridLayout>
#include <QLabel>
#include <QCheckBox>
#include <QSpinBox>

class Sidebar : public QWidget
{
    Q_OBJECT

public:
    explicit Sidebar(QWidget* parent = nullptr);

    // Accessors if you want to read values externally
    QCheckBox* checkBox(const QString& key) const;
    QSpinBox* spinBox(const QString& key) const;

private:
    void addCheckBox(const QString& label, const QString& key, bool defaultValue);
    void addSpinBox(const QString& label, const QString& key, int min, int max, int defaultValue);

    QGridLayout* m_layout;
    int m_row = 0;

    // Optional: store widgets by key for easy lookup
    QMap<QString, QCheckBox*> m_checkBoxes;
    QMap<QString, QSpinBox*> m_spinBoxes;
};

#endif // SIDEBAR_H
