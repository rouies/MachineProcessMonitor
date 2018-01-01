#include "processlistener.h"

#include "qdebug.h"

#include <string>
#include <QDir>
#include <QNetworkInterface>
#include <QNetworkRequest>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

void writeByteArray(QByteArray& arr,qint64 time,uchar iclose)
{
    arr.clear();
    QDataStream out(&arr,QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_10);
    out.setByteOrder(QDataStream::LittleEndian);
    out << time << iclose;
}

void readByteArray(QByteArray& arr,qint64& time,uchar& iclose)
{
    QDataStream out(&arr,QIODevice::ReadOnly);
    out.setVersion(QDataStream::Qt_5_10);
    out.setByteOrder(QDataStream::LittleEndian);
    out >> time >> iclose;
}

ProcessListener::ProcessListener(int queueSize,QObject *parent) : QObject(parent)
{
    this->processTimer = new QTimer(this);
    this->uploadTimer = new QTimer(this);
    this->processTimer->setTimerType(Qt::PreciseTimer);
    this->uploadTimer->setTimerType(Qt::CoarseTimer);
    this->queueSize = queueSize;
    this->filterSize = 0;
    this->connect(this->processTimer,SIGNAL(timeout()),this,SLOT(processTimerTimeout()));
    this->connect(this->uploadTimer,SIGNAL(timeout()),this,SLOT(uploadTimerTimeout()));
    this->connect(&this->httpClient,SIGNAL(finished(QNetworkReply*)),this,SLOT(httpFinished(QNetworkReply*)));
}

ProcessListener::~ProcessListener()
{
    this->processTimer->stop();
    this->uploadTimer->stop();
    for(int i=0;i<this->filterSize;i++)
    {
        delete this->filters[i]->processName;
    }
    qDeleteAll(this->filters);
    this->filters.clear();
    qDeleteAll(this->queueMapping);
    this->queueMapping.clear();
    qDeleteAll(this->lockMapping);
    this->lockMapping.clear();
}

void ProcessListener::setNetworkInfo(QString url, QString name)
{
    networkInfo.requestUrl = QUrl(url);
    QNetworkInterface interface = QNetworkInterface::interfaceFromName(name);
    if(interface.isValid())
    {
        QJsonObject json;
        this->networkInfo.macAddress = interface.hardwareAddress();
        json.insert("Mac",this->networkInfo.macAddress);
        this->networkInfo.hostName = QHostInfo::localHostName();
        json.insert("HostName",this->networkInfo.hostName);
        QJsonArray addressList;
        QList<QNetworkAddressEntry> entries = interface.addressEntries();
        for(int i=0;i<entries.size();i++)
        {
            QNetworkAddressEntry entry = entries.at(i);
            IPAddressInfo address;
            address.address =  entry.ip().toString();
            address.broadcast = entry.broadcast().toString();
            address.netmask = entry.netmask().toString();
            this->networkInfo.addresses << address;
            QJsonObject addressInfo;
            addressInfo.insert("address",address.address);
            addressInfo.insert("broadcast",address.broadcast);
            addressInfo.insert("netmask",address.netmask);
            addressList.append(addressInfo);
        }
        json.insert("addresses",addressList);
        QJsonDocument doc;
        doc.setObject(json);
        QByteArray data = doc.toJson(QJsonDocument::Compact);
        this->httpHeader = data;
        /*
        QNetworkRequest request(this->networkInfo.requestUrl);
        QNetworkReply* reply = this->httpClient.get(request);
        reply->setProperty("QueueIndex",100);
        reply->setProperty("Count",200);
        */
    }
}

void ProcessListener::appendProcess(QString processName, QString filePath)
{
    //创建监控程序
    QByteArray pn = processName.toLocal8Bit();
    ProcessFilter* filter = new ProcessFilter;
    char* data = pn.data();
    char* pns = new char[pn.size() + 1];
    pns[pn.size()] = '\0';
#ifdef Q_OS_MACX
    strcpy(pns,data);
#endif
#ifdef Q_OS_WIN32
    strcyp_s(pns,pn.size()+1,data);
#endif

    filter->processName = pns;
    filter->isClose = false;
    this->filters << filter;
#ifdef Q_OS_MACX
    processName = processName.section('/',-1);
#endif
#ifdef Q_OS_WIN32
    processName = processName.section('.',-2);
#endif

    //创建传输队列
    QString queuePath = QString("%1%2.queue").arg(filePath).arg(processName);
    FileRecordQueue* queue = FileRecordQueue::createFileRecordQueue(processName,queuePath,sizeof(qint64) + 1,this->queueSize,this);
    this->queueMapping.insert(filter,queue);
    //创建文件锁
    QString lockPath = QString("%1%2.lock").arg(filePath).arg(processName);
    LockFile* lock = new LockFile(lockPath,this);
    this->lockMapping.insert(filter,lock);
    this->filterSize++;
}


void ProcessListener::processTimerTimeout()
{
    this->manager.checkProcessIsClose(this->filters);
    for(int i=0 ; i < this->filterSize; i++)
    {
        bool cic = this->filters[i]->isClose;
        LockFile* lockFile = this->lockMapping[this->filters[i]];
        FileRecordQueue* queue = this->queueMapping[this->filters[i]];
        qint64 time = 0;
        uchar isClose = 0;
        lockFile->ReadRefurbishTime(&time,&isClose);
        qint64 ctime = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();
        if((cic ? 1 : 0) == isClose)
        {
            //如果状态都是开启检查更新时间
            if(!cic && (ctime - time > this->ptTimeout + 60000))
            {
                //认为是休眠
                QByteArray arr;
                writeByteArray(arr,time,1);
                queue->enquque(arr);
                writeByteArray(arr,ctime,0);
                queue->enquque(arr);
            }
        }
        else
        {
            //如果状态是关闭，上次是开启，检查更新时间
            if(cic)
            {
                if(ctime - time > this->ptTimeout + 60000)
                {
                    //认为是关机
                    QByteArray arr;
                    writeByteArray(arr,time,1);
                    queue->enquque(arr);
                }
                else
                {
                    QByteArray arr;
                    writeByteArray(arr,ctime,1);
                    queue->enquque(arr);
                }
            }
            else
            {
                QByteArray arr;
                writeByteArray(arr,ctime,0);
                queue->enquque(arr);
            }
        }
        lockFile->WriteRefurbishTime(ctime,(cic ? 1 : 0));
    }

    //qDebug() << QDateTime::currentDateTimeUtc().toString("yyyy-MM-dd HH:mm:ss") << "\n";
}

void ProcessListener::uploadTimerTimeout()
{
    for(int i=0 ; i < this->filterSize; i++)
    {
        FileRecordQueue* queue = this->queueMapping[this->filters[i]];
        if(queue->size() > 0)
        {
            QHostInfo::lookupHost(this->networkInfo.requestUrl.host(),this,SLOT(checkNetwork(QHostInfo)));
            break;
        }
    }
}

void ProcessListener::startProcessTimer(int timeout)
{
    for(int i = 0 ;i< this->filterSize ;i++)
    {
        FileRecordQueue* queue = this->queueMapping[this->filters[i]];
        LockFile* lockFile = this->lockMapping[this->filters[i]];
        lockFile->open();
        queue->open();
    }
    this->ptTimeout = timeout;
    this->processTimer->start(timeout);

}

void ProcessListener::startUploadTimer(int timeout)
{
    this->upTimeout = timeout;
    this->uploadTimer->start(timeout);
}

void ProcessListener::checkNetwork(const QHostInfo &host)
{
    if(host.error() == QHostInfo::NoError)
    {
        for(int i=0 ; i < this->filterSize; i++)
        {
            FileRecordQueue* queue = this->queueMapping[this->filters[i]];
            if(queue->size() > 0)
            {
                int error = 0;
                QByteArray data;
                int count = queue->get(data,&error,0);
                if(!error)
                {
                    QNetworkRequest request(this->networkInfo.requestUrl);
                    request.setRawHeader(QString("ProcessName").toUtf8(),queue->queueName().toUtf8());
                    request.setRawHeader(QString("Mac").toUtf8(),this->networkInfo.macAddress.toUtf8());
                    request.setRawHeader(QString("Client-info").toUtf8(),this->httpHeader);
                    QNetworkReply* reply = this->httpClient.post(request,data);
                    reply->setProperty("QueueIndex",i);
                    reply->setProperty("Count",count);
                }
            }
        }
    }
}

void ProcessListener::httpFinished(QNetworkReply* reply)
{
    if(reply->error() == QNetworkReply::NoError)
    {
        int index = reply->property("QueueIndex").toInt();
        int count = reply->property("Count").toInt();
        FileRecordQueue* queue = this->queueMapping[this->filters[index]];
        queue->removeQueueHeader(count);
    }
    reply->deleteLater();

}
