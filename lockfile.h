#ifndef LOGFILE_H
#define LOGFILE_H

#include <QObject>
#include <QFile>
#include <QDataStream>
#include <QDateTime>

class LockFile : public QObject
{
    Q_OBJECT
public:
    explicit LockFile(QString filePath,QObject *parent = nullptr);
    ~LockFile();
    int open();
    void closeFile();
    int WriteRefurbishTime(QDateTime date,uchar isClose);
    int ReadRefurbishTime(QDateTime* date,uchar* isClose);
    int WriteRefurbishTime(qint64 msecs,uchar isClose);
    int ReadRefurbishTime(qint64* msecs,uchar* isClose);
private:
    QFile* file;
signals:

public slots:
};

#endif // LOGFILE_H
