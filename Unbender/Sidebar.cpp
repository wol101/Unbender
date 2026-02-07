#include "Sidebar.h"

Sidebar::Sidebar(QWidget* parent)
    : QWidget(parent)
{
    m_layout = new QGridLayout(this);
    m_layout->setColumnStretch(0, 0);
    m_layout->setColumnStretch(1, 1);
    m_layout->setContentsMargins(6, 6, 6, 6);
    m_layout->setHorizontalSpacing(10);
    m_layout->setVerticalSpacing(6);

    //
    // Example controls — replace or extend as needed
    //
    addCheckBox("Enable Shadows", "enableShadows", true);
    addCheckBox("Wireframe Mode", "wireframe", false);

    addSpinBox("Detail Level", "detailLevel", 1, 10, 5);
    addSpinBox("Max Iterations", "maxIterations", 1, 1000, 100);


    // Add spacer to force top alignment
    //
    m_layout->setRowStretch(m_row, 1);

}

void Sidebar::addCheckBox(const QString& label, const QString& key, bool defaultValue)
{
    auto* lbl = new QLabel(label, this);
    auto* cb  = new QCheckBox(this);
    cb->setChecked(defaultValue);

    m_layout->addWidget(lbl, m_row, 0);
    m_layout->addWidget(cb,  m_row, 1);

    m_checkBoxes.insert(key, cb);
    m_row++;
}

void Sidebar::addSpinBox(const QString& label, const QString& key,
                         int min, int max, int defaultValue)
{
    auto* lbl = new QLabel(label, this);
    auto* sb  = new QSpinBox(this);

    sb->setRange(min, max);
    sb->setValue(defaultValue);

    m_layout->addWidget(lbl, m_row, 0);
    m_layout->addWidget(sb,  m_row, 1);

    m_spinBoxes.insert(key, sb);
    m_row++;
}

QCheckBox* Sidebar::checkBox(const QString& key) const
{
    return m_checkBoxes.value(key, nullptr);
}

QSpinBox* Sidebar::spinBox(const QString& key) const
{
    return m_spinBoxes.value(key, nullptr);
}
