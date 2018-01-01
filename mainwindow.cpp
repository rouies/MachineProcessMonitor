#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "processmanager.h"


#include <QMessageBox>
#include <QDataStream>
#include <QSettings>

#include "processlistener.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    //QString dirPath = QCoreApplication::applicationDirPath();
    //QSettings config(QString("%1/%2").arg(dirPath).arg("config.ini"), QSettings::IniFormat);
    QSettings config("config.ini", QSettings::IniFormat);
    int queueCapacity = config.value("/application/QueueCapacity").toInt();
    QString savePath = config.value("/application/SaveFilePath").toString();
    QString netname = config.value("/application/NetworkDeviceName").toString();

    QString prnamestring = config.value("/process/ListenProcessNames").toString();
    int procPeriod = config.value("/process/ProcessScanPeriod").toInt();

    QString uploadUrl = config.value("/upload/UploadFileUrl").toString();
    int upPeriod = config.value("/upload/UploadFileScanPeriod").toInt();

    this->listener = new ProcessListener(queueCapacity,this);
    listener->setNetworkInfo(uploadUrl,netname);
    QStringList processList = prnamestring.split(',');
    for(int i=0;i<processList.size();i++)
    {
        listener->appendProcess(processList[i],savePath);
    }
    listener->startProcessTimer(procPeriod);
    listener->startUploadTimer(upPeriod);
}

MainWindow::~MainWindow()
{
    this->listener->deleteLater();
    delete ui;
}

