#include "PathEditWidget.h"
#include <QHBoxLayout>
#include <QFileDialog>
#include <QFileInfo>
#include <QSettings>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QUrl>

PathEditWidget::PathEditWidget(Mode mode, const QString &buttonText, QWidget* parent)
    : QWidget(parent),
    m_mode(mode)
{
    setAcceptDrops(true);

    m_edit = new QLineEdit(this);
    m_button = new QPushButton(buttonText, this);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0,0,0,0);
    layout->addWidget(m_edit);
    layout->addWidget(m_button);

    connect(m_button, &QPushButton::clicked, this, &PathEditWidget::browse);
    connect(m_edit, &QLineEdit::textChanged, this, &PathEditWidget::validatePath);

    validatePath();
}

QString PathEditWidget::path() const
{
    return m_edit->text();
}

void PathEditWidget::setPath(const QString& p)
{
    m_edit->setText(p);
    validatePath();
}

void PathEditWidget::browse()
{
    QString selected;

    if (m_mode == FileMode) {
        selected = QFileDialog::getOpenFileName(this, "Select File", path());
    } else {
        selected = QFileDialog::getExistingDirectory(this, "Select Folder", path());
    }

    if (!selected.isEmpty()) {
        m_edit->setText(selected);
        validatePath();
        emit pathChanged(selected);
    }
}

void PathEditWidget::validatePath()
{
    QFileInfo info(m_edit->text());
    bool valid = false;

    if (m_mode == FileMode)
        valid = info.exists() && info.isFile();
    else
        valid = info.exists() && info.isDir();

    updateStyle(valid);

    emit pathChanged(m_edit->text());
}

void PathEditWidget::updateStyle(bool valid)
{
    if (valid) {
        m_edit->setStyleSheet("");
    } else {
        m_edit->setStyleSheet("QLineEdit { background-color: #ffcccc; }");
    }
}

//
// Drag & Drop
//

void PathEditWidget::dragEnterEvent(QDragEnterEvent* event)
{
    if (!event->mimeData()->hasUrls())
        return;

    const QList<QUrl> urls = event->mimeData()->urls();
    if (urls.isEmpty())
        return;

    QFileInfo info(urls.first().toLocalFile());

    bool acceptable = false;

    if (m_mode == FileMode)
        acceptable = info.exists() && info.isFile();
    else
        acceptable = info.exists() && info.isDir();

    if (acceptable)
        event->acceptProposedAction();
}

void PathEditWidget::dropEvent(QDropEvent* event)
{
    const QList<QUrl> urls = event->mimeData()->urls();
    if (urls.isEmpty())
        return;

    QString p = urls.first().toLocalFile();
    m_edit->setText(p);
    validatePath();
    emit pathChanged(p);
}
