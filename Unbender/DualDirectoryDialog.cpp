#include "DualDirectoryDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDir>
#include <QMessageBox>
#include <QRegularExpression>


DualDirectoryDialog::DualDirectoryDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Select Input and Output Folders");

    auto *mainLayout = new QVBoxLayout(this);

    // -----------------------------
    // Input folder dialog
    // -----------------------------
    mainLayout->addWidget(new QLabel("Select Input Folder:", this));

    inputDialog = new QFileDialog(this);
    inputDialog->setOption(QFileDialog::DontUseNativeDialog, true);
    inputDialog->setFileMode(QFileDialog::Directory);
    inputDialog->setOption(QFileDialog::ShowDirsOnly, true);
    inputDialog->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    inputDialog->setWindowFlags(inputDialog->windowFlags() & ~Qt::Dialog);

    connect(inputDialog, &QFileDialog::fileSelected, this, &DualDirectoryDialog::onInputDirSelected);

    mainLayout->addWidget(inputDialog);

    inputLineEdit = new QLineEdit(this);
    mainLayout->addWidget(new QLabel("Input Folder:", this));
    mainLayout->addWidget(inputLineEdit);

    // -----------------------------
    // Output folder dialog
    // -----------------------------
    mainLayout->addWidget(new QLabel("Select Output Folder:", this));

    outputDialog = new QFileDialog(this);
    outputDialog->setOption(QFileDialog::DontUseNativeDialog, true);
    outputDialog->setFileMode(QFileDialog::Directory);
    outputDialog->setOption(QFileDialog::ShowDirsOnly, true);
    outputDialog->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    outputDialog->setWindowFlags(inputDialog->windowFlags() & ~Qt::Dialog);

    connect(outputDialog, &QFileDialog::fileSelected, this, &DualDirectoryDialog::onOutputDirSelected);

    mainLayout->addWidget(outputDialog);

    outputLineEdit = new QLineEdit(this);
    mainLayout->addWidget(new QLabel("Output Folder:", this));
    mainLayout->addWidget(outputLineEdit);

    // -----------------------------
    // OK / Cancel buttons
    // -----------------------------
    auto *buttonLayout = new QHBoxLayout();

    okButton = new QPushButton("OK", this);
    cancelButton = new QPushButton("Cancel", this);

    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);

    mainLayout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked,
            this, &DualDirectoryDialog::onAccept);

    connect(cancelButton, &QPushButton::clicked,
            this, &DualDirectoryDialog::reject);

    // Load persistent settings
    loadSettings();
}

DualDirectoryDialog::~DualDirectoryDialog()
{
    saveSettings();
}

void DualDirectoryDialog::onInputDirSelected(const QString &path)
{
    inputLineEdit->setText(path);
}

void DualDirectoryDialog::onOutputDirSelected(const QString &path)
{
    outputLineEdit->setText(path);
}

void DualDirectoryDialog::onAccept()
{
    QString outDir = outputLineEdit->text();

    if (outDir.isEmpty()) {
        QMessageBox::warning(this, "Invalid Output Folder",
                             "Please select an output folder.");
        return;
    }

    QFileInfo dir(outDir);

    if (!dir.exists()) {
        QMessageBox::warning(this, "Invalid Output Folder",
                             "The selected output folder does not exist.");
        return;
    }

    if (!dir.isReadable() || !dir.isWritable()) {
        QMessageBox::warning(this, "Output Folder Not Writable",
                             "You do not have write permission for the selected output folder.");
        return;
    }

    accept();
}

void DualDirectoryDialog::loadSettings()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "AnimalSimulationLaboratory", "Unbender");
    QString lastInput  = settings.value("inputFolder").toString();
    QString lastOutput = settings.value("outputFolder").toString();

    if (!lastInput.isEmpty()) {
        inputLineEdit->setText(lastInput);
        inputDialog->setDirectory(lastInput);
    }

    if (!lastOutput.isEmpty()) {
        outputLineEdit->setText(lastOutput);
        outputDialog->setDirectory(lastOutput);
    }

    QByteArray geom = settings.value("windowGeometryDualDirectoryDialog").toByteArray();
    if (!geom.isEmpty()) {
        restoreGeometry(geom);
    }

}

void DualDirectoryDialog::saveSettings()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "AnimalSimulationLaboratory", "Unbender");
    settings.setValue("inputFolder",  inputLineEdit->text());
    settings.setValue("outputFolder", outputLineEdit->text());
    settings.setValue("windowGeometryDualDirectoryDialog", saveGeometry());
}


QStringList DualDirectoryDialog::filesMatchingRegex(const QString &folder, const QString &pattern)
{
    QDir dir(folder);
    QStringList allFiles = dir.entryList(QDir::Files);

    QRegularExpression re(pattern);
    QStringList matched;

    for (auto &&file : allFiles)
    {
        if (re.match(file).hasMatch()) {
            matched << dir.filePath(file);
        }
    }

    return matched;
}
