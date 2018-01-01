#ifndef PROCESSLISTENER_H
#define PROCESSLISTENER_H

#include <QObject>
#include <QTimer>
#include <QMap>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QHostInfo>


#include "processmanager.h"
#include "filerecordqueue.h"
#include "lockfile.h"

typedef struct {
    QString address;
    QString netmask;
    QString broadcast;
} IPAddressInfo;

typedef struct {
    QUrl requestUrl;
    QList<IPAddressInfo> addresses;
    QString macAddress;
    QString hostName;
} NetworkInfo;

class ProcessListener : public QObject
{
    Q_OBJECT
public:
    explicit ProcessListener(int queueSize,QObject *parent = nullptr);
    ~ProcessListener();
    void appendProcess(QString processName,QString filePath);
    void startProcessTimer(int timeout);
    void startUploadTimer(int timeout);
    void setNetworkInfo(QString url,QString name);
private:
    NetworkInfo networkInfo;
    QTimer* processTimer;
    QTimer* uploadTimer;
    int ptTimeout;
    int upTimeout;
    int queueSize;
    int filterSize;
    ProcessManager manager;
    QNetworkAccessManager httpClient;
    QList<ProcessFilter*> filters;
    QMap<ProcessFilter*,FileRecordQueue*> queueMapping;
    QMap<ProcessFilter*,LockFile*> lockMapping;
    QByteArray httpHeader;
    void postRequest(QByteArray arr);
signals:

private slots:
    void processTimerTimeout();
    void uploadTimerTimeout();
    void checkNetwork(const QHostInfo &host);
    void httpFinished(QNetworkReply*);
};

#endif // PROCESSLISTENER_H
