#include "filerecordqueue.h"
#include "qdebug.h"

FileRecordQueue::FileRecordQueue(QString name,QString filePath,QObject *parent) : QObject(parent)
{
    this->name = name;
    this->queueFile = new QFile(filePath);
    this->semaphore = new QSemaphore(1);
    this->timeout = 3000;
}

FileRecordQueue::~FileRecordQueue()
{
    if(this->queueFile->isOpen())
    {
        this->queueFile->close();
    }
    delete this->queueFile;

    delete this->semaphore;
}

QString FileRecordQueue::queueName()
{
    return this->name;
}

int FileRecordQueue::open()
{
    if(!this->queueFile->open(QIODevice::ReadWrite))
    {
        return FILE_RECORD_QUEUE_FILE_OPEN_ERROR;
    }
    QDataStream queueStream(this->queueFile);
    queueStream.setVersion(QDataStream::Qt_5_10);
    queueStream.setByteOrder(QDataStream::LittleEndian);
    //获取队列头信息
    queueStream >> this->queueCount;
    queueStream >> this->itemSize;
    queueStream >> this->de;
    queueStream >> this->en;
    this->fileSize = this->itemSize * this->queueCount + 16;
    return FILE_RECORD_QUEUE_SUCCESS;
}

void FileRecordQueue::close()
{
    if(this->queueFile->isOpen())
    {
        this->queueFile->close();
    }
}


FileRecordQueue* FileRecordQueue::createFileRecordQueue(QString name,QString filePath,int itemSize,int queueCount,QObject *parent)
{
    QFile file(filePath);
    if(!file.exists())
    {
        if(file.open(QIODevice::WriteOnly)){
            //开头四个项 队列总数 单元大小 队头指针 队尾指针
            QDataStream out(&file);
            out.setVersion(QDataStream::Qt_5_10);
            out.setByteOrder(QDataStream::LittleEndian);
            out << (qint32)queueCount << (qint32)itemSize << (qint32)0 << (qint32)0;
            file.resize(queueCount * itemSize + 16);
            file.close();
        } else {
            return nullptr;
        }
    }
    return new FileRecordQueue(name,filePath,parent);

}

qint64 FileRecordQueue::size()
{
    if(this->en == this->de)
    {
        return 0;
    }
    else if(this->en > this->de)
    {
        return this->en - this->de;
    }
    else
    {
        return this->queueCount - (this->de - this->en);
    }
}


int FileRecordQueue::enquque(QByteArray& item)
{
    if(!this->semaphore->tryAcquire(1,this->timeout))
    {
        return FILE_RECORD_QUEUE_SEMAPHORE_TIMEOUT;//超时
    }
    if(item.size() != this->itemSize)
    {
        this->semaphore->release(1);
        return FILE_RECORD_QUEUE_ENQUEUE_SIZE_ERROR;
    }
    if((this->en + 1) % this->queueCount == this->de)
    {
        this->semaphore->release(1);
        return FILE_RECORD_QUEUE_ENQUEUE_IS_FULL;
    }
    if(!this->queueFile->seek(this->en * this->itemSize + 16))
    {
        this->semaphore->release(1);
        return FILE_RECORD_QUEUE_FILE_SEEK_ERROR;
    }
    if(this->queueFile->write(item.data(),this->itemSize) != this->itemSize)
    {
        this->semaphore->release(1);
        return FILE_RECORD_QUEUE_FILE_RW_ERROR;
    }
    if(!this->queueFile->flush())
    {
        this->semaphore->release(1);
        return FILE_RECORD_QUEUE_FILE_RW_ERROR;
    }
    int cren = this->en;
    cren = (cren + 1) % this->queueCount;
    //更新队尾指针
    if(!this->queueFile->seek(sizeof(int) * 3))
    {
        this->semaphore->release(1);
        return FILE_RECORD_QUEUE_FILE_SEEK_ERROR;
    }

    QDataStream queueStream(this->queueFile);
    queueStream.setVersion(QDataStream::Qt_5_10);
    queueStream.setByteOrder(QDataStream::LittleEndian);
    queueStream << cren;
    if(!this->queueFile->flush() || this->queueFile->error() != QFile::NoError)
    {
        this->semaphore->release(1);
        return FILE_RECORD_QUEUE_FILE_MODIFY_PTR_ERROR;
    }
    this->en = cren;
    this->semaphore->release(1);
    emit this->onEnQueue(this);
    return FILE_RECORD_QUEUE_SUCCESS;
}

QByteArray FileRecordQueue::dequeue(int* error)
{
    if(!this->semaphore->tryAcquire(1,this->timeout))
    {
        if(error != nullptr)
        {
            *error = FILE_RECORD_QUEUE_SEMAPHORE_TIMEOUT;//超时
        }
        return QByteArray();
    }
    else
    {
        if(this->en == this->de)
        {
            if(error != nullptr)
            {
                *error = FILE_RECORD_QUEUE_DEQUEUE_IS_EMPTY;//空队列
            }
            this->semaphore->release(1);
            return QByteArray();
        }
        qint64 len = this->itemSize;
        if(!this->queueFile->seek(this->de * this->itemSize + 16))
        {
            if(error != nullptr)
            {
                *error = FILE_RECORD_QUEUE_FILE_SEEK_ERROR;
            }
            this->semaphore->release(1);
            return QByteArray();
        }
        QByteArray res = this->queueFile->read(len);
        if(this->queueFile->error() != QFile::NoError)
        {
            if(error != nullptr)
            {
                *error = FILE_RECORD_QUEUE_FILE_RW_ERROR;
            }
            this->semaphore->release(1);
            return QByteArray();
        }
        int crde = this->de;
        crde = (crde + 1) % this->queueCount;
        //更新队头指针
        if(!this->queueFile->seek(sizeof(int) * 2))
        {
            if(error != nullptr)
            {
                *error = FILE_RECORD_QUEUE_FILE_SEEK_ERROR;
            }
            this->semaphore->release(1);
            return QByteArray();
        }
        QDataStream queueStream(this->queueFile);
        queueStream.setVersion(QDataStream::Qt_5_10);
        queueStream.setByteOrder(QDataStream::LittleEndian);
        queueStream << crde;
        if(!this->queueFile->flush() || this->queueFile->error() != QFile::NoError)
        {
            if(error != nullptr)
            {
                *error = FILE_RECORD_QUEUE_FILE_MODIFY_PTR_ERROR;
            }
            this->semaphore->release(1);
            return QByteArray();
        }
        this->de = crde;
        if(error != nullptr)
        {
            *error = FILE_RECORD_QUEUE_SUCCESS;
        }
        this->semaphore->release(1);
        return res;
    }

}

int FileRecordQueue::get(QList<QByteArray>& list,int* error,int size)
{
    if(!this->semaphore->tryAcquire(1,this->timeout))
    {
        if(error != nullptr)
        {
            *error = FILE_RECORD_QUEUE_SEMAPHORE_TIMEOUT;//超时
        }
        return 0;
    }
    else
    {
        list.clear();
        if(size <= 0)
        {
            size = this->size();
        }
        if(this->size() < size)
        {
            if(error != nullptr)
            {
                *error = FILE_RECORD_QUEUE_GET_SIZE_TOO_LARGE;
            }
            this->semaphore->release(1);
            return 0;
        }
        int crde = this->de;
        qint64 len = this->itemSize;
        this->queueFile->seek(crde * this->itemSize + 16);
        for(int i=0 ; i<size ; i++)
        {
            QByteArray res = this->queueFile->read(len);
            if(this->queueFile->error() != QFile::NoError)
            {
                if(error != nullptr)
                {
                    *error = FILE_RECORD_QUEUE_FILE_RW_ERROR;
                }
                this->semaphore->release(1);
                return 0;
            }
            crde = (crde + 1) % this->queueCount;
            if(crde == 0)
            {
                if(!this->queueFile->seek(16))
                {
                    if(error != nullptr)
                    {
                        *error = FILE_RECORD_QUEUE_FILE_SEEK_ERROR;
                    }
                    this->semaphore->release(1);
                    return 0;
                }
            }
            list << res;
        }
        if(error != nullptr)
        {
            *error = FILE_RECORD_QUEUE_SUCCESS;
        }
        this->semaphore->release(1);
        return size;
    }
}

int FileRecordQueue::get(QByteArray& data,int* error,int size)
{
    if(!this->semaphore->tryAcquire(1,this->timeout))
    {
        if(error != nullptr)
        {
            *error = FILE_RECORD_QUEUE_SEMAPHORE_TIMEOUT;//超时
        }
        return 0;
    }
    else
    {
        data.clear();
        if(size <= 0)
        {
            size = this->size();
        }
        if(this->size() < size)
        {
            if(error != nullptr)
            {
                *error = FILE_RECORD_QUEUE_GET_SIZE_TOO_LARGE;
            }
            this->semaphore->release(1);
            return 0;
        }
        int crde = this->de;
        qint64 len = this->itemSize;
        this->queueFile->seek(crde * this->itemSize + 16);
        for(int i=0 ; i<size ; i++)
        {
            QByteArray res = this->queueFile->read(len);
            if(this->queueFile->error() != QFile::NoError)
            {
                if(error != nullptr)
                {
                    *error = FILE_RECORD_QUEUE_FILE_RW_ERROR;
                }
                this->semaphore->release(1);
                return 0;
            }
            crde = (crde + 1) % this->queueCount;
            if(crde == 0)
            {
                if(!this->queueFile->seek(16))
                {
                    if(error != nullptr)
                    {
                        *error = FILE_RECORD_QUEUE_FILE_SEEK_ERROR;
                    }
                    this->semaphore->release(1);
                    return 0;
                }
            }
            data.append(res);
        }
        if(error != nullptr)
        {
            *error = FILE_RECORD_QUEUE_SUCCESS;
        }
        this->semaphore->release(1);
        return size;
    }
}

int FileRecordQueue::removeQueueHeader(int size)
{
    if(!this->semaphore->tryAcquire(1,this->timeout))
    {
        return FILE_RECORD_QUEUE_SEMAPHORE_TIMEOUT;//超时
    }
    if(size > this->size())
    {
        this->semaphore->release(1);
        return FILE_RECORD_QUEUE_GET_SIZE_TOO_LARGE;
    }
    int crde = this->de;
    crde = (crde + size) % this->queueCount;
    //更新队头指针
    if(!this->queueFile->seek(sizeof(int) * 2))
    {
        this->semaphore->release(1);
        return FILE_RECORD_QUEUE_FILE_SEEK_ERROR;
    }
    QDataStream queueStream(this->queueFile);
    queueStream.setVersion(QDataStream::Qt_5_10);
    queueStream.setByteOrder(QDataStream::LittleEndian);
    queueStream << crde;
    if(!this->queueFile->flush() || this->queueFile->error() != QFile::NoError)
    {
        this->semaphore->release(1);
        return FILE_RECORD_QUEUE_FILE_MODIFY_PTR_ERROR;
    }
    this->de = crde;
    this->semaphore->release(1);
    return FILE_RECORD_QUEUE_SUCCESS;
}


