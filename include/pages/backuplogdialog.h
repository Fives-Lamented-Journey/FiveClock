#ifndef BACKUPLOGDIALOG_H
#define BACKUPLOGDIALOG_H

#include <QDialog>
namespace Ui {
    class BackupLogDialog;
}

class BackupLogDialog : public QDialog
{
    Q_OBJECT
public:
    explicit BackupLogDialog(QWidget *parent = nullptr);
    ~BackupLogDialog();

public slots:
    void appendLog(QString msg);
    void setProgress(int val);
    void clearAll();

signals:
    void cancelRequested();

private slots:
    void onCancelClicked();

private:
    Ui::BackupLogDialog *ui;
};

#endif // BACKUPLOGDIALOG_H
