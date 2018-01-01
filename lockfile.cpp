#include "lockfile.h"
#include <QDate>

LockFile::LockFile(QString filePath,QObject *parent) : QObject(parent)
{
    this->file = new QFile(filePath,this);
}

LockFile::~LockFile(){
    if(this->file->isOpen())
    {
        this->file->close();
    }
    delete this->file;
    this->file = nullptr;

}

int LockFile::open(){
    bool isExists = this->file->exists();
    if(!this->file->open(QIODevice::ReadWrite))
    {
        return 1;
    }
    if(!isExists)
    {
        this->WriteRefurbishTime(QDateTime::currentDateTimeUtc(),1);
    }
    return 0;
}


void LockFile::closeFile(){
    if(this->file!=nullptr && this->file->isOpen())
    {
        this->file->close();
    }
}

int LockFile::WriteRefurbishTime(qint64 msecs,uchar isClose){
    if(this->file == nullptr){
        return 3;
    }
    if(!this->file->isOpen()){
        return 4;
    }
    if(!this->file->seek(0)){
       return 5;
    }
    this->file->seek(0);
    QDataStream out(this->file);
    out.setByteOrder(QDataStream::LittleEndian);
    out.setVersion(QDataStream::Qt_5_10);
    out << msecs << isClose;
    this->file->flush();
    return 0;
}

int LockFile::ReadRefurbishTime(qint64 *msecs,uchar* isClose){
    if(this->file == nullptr){
        return 3;
    }
    if(!this->file->isOpen()){
        return 4;
    }
    if(!this->file->seek(0)){
       return 5;
    }
    this->file->seek(0);
    QDataStream out(this->file);
    out.setByteOrder(QDataStream::LittleEndian);
    out.setVersion(QDataStream::Qt_5_10);
    out >> *msecs;
    out >> *isClose;
    return 0;
}

int LockFile::WriteRefurbishTime(QDateTime date,uchar isClose){
    qint64 time = date.toMSecsSinceEpoch();
    return this->WriteRefurbishTime(time,isClose);
}

int LockFile::ReadRefurbishTime(QDateTime *date,uchar* isClose){
    qint64 msecs  = 0;
    int res = this->ReadRefurbishTime(&msecs,isClose);
    if(res == 0){
        date->setMSecsSinceEpoch(msecs);
    }
    return res;
}

