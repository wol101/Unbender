#ifndef SIDEBAR_H
#define SIDEBAR_H

#include "PathEditWidget.h"

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

    void addCheckBox(const QString& label, const QString& key, bool defaultValue);
    void addSpinBox(const QString& label, const QString& key, int min, int max, int defaultValue);
    void addPathEditWidget(const QString& label, const QString& key, PathEditWidget::Mode mode, QString startPath);
    void addSpacer();

    QCheckBox* checkBox(const QString& key) const;
    QSpinBox* spinBox(const QString& key) const;
    PathEditWidget* pathEditWidget(const QString& key) const;

private:
    QGridLayout* m_layout;
    int m_row = 0;

    QMap<QString, QCheckBox*> m_checkBoxes;
    QMap<QString, QSpinBox*> m_spinBoxes;
    QMap<QString, PathEditWidget*> m_pathEditWidgets;
};

#endif // SIDEBAR_H
