#include "include/pages/backuplogdialog.h"
#include "ui_backuplogdialog.h"
#include <QDateTime>



BackupLogDialog::BackupLogDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::BackupLogDialog)
{
    ui->setupUi(this);
    ui->textEdit->setReadOnly(true);
    // 设置：非模态，打开后主窗口仍然可以操作
    setModal(false);
    // 窗口打开自动居中到父窗口（homePage）
    connect(ui->cancelButton,&QPushButton::clicked,this,&BackupLogDialog::onCancelClicked);

}

BackupLogDialog::~BackupLogDialog()
{
    delete ui;
}

void BackupLogDialog::clearAll()
{
    qDebug() << __func__;
    ui->textEdit->clear();
    ui->progressBar->setValue(0);
}


void BackupLogDialog::appendLog(QString msg)
{
    qDebug() << __func__;
    QString tm = QDateTime::currentDateTime().toString("HH:mm:ss");
    ui->textEdit->append(QString("[%1] %2").arg(tm,msg));
}

void BackupLogDialog::setProgress(int val)
{
    qDebug() << __func__;
    ui->progressBar->setValue(val);
}

void BackupLogDialog::onCancelClicked()
{
    emit cancelRequested();
    appendLog("⚠️ 用户请求取消任务...");
}
