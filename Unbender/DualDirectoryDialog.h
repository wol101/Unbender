#ifndef DUALDIRECTORYDIALOG_H
#define DUALDIRECTORYDIALOG_H

#pragma once

#include <QDialog>
#include <QFileDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>


class DualDirectoryDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DualDirectoryDialog(QWidget *parent = nullptr);
    ~DualDirectoryDialog();

    QString inputFolder() const { return inputLineEdit->text(); }
    QString outputFolder() const { return outputLineEdit->text(); }

    static QStringList filesMatchingRegex(const QString &folder, const QString &pattern);

private slots:
    void onInputDirSelected(const QString &path);
    void onOutputDirSelected(const QString &path);
    void onAccept();

private:
    void loadSettings();
    void saveSettings();

    QFileDialog *inputDialog;
    QFileDialog *outputDialog;

    QLineEdit *inputLineEdit;
    QLineEdit *outputLineEdit;

    QPushButton *okButton;
    QPushButton *cancelButton;
};

#endif // DUALDIRECTORYDIALOG_H
