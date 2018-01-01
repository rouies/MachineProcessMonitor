#ifndef FILERECORDQUEUE_H
#define FILERECORDQUEUE_H

#include <QObject>
#include <QFile>
#include <QDataStream>
#include <QSemaphore>

//成功
#define FILE_RECORD_QUEUE_SUCCESS                  (0)
//文件打开错误
#define FILE_RECORD_QUEUE_FILE_OPEN_ERROR          (1)
//信号量获取等待超时
#define FILE_RECORD_QUEUE_SEMAPHORE_TIMEOUT        (2)
//出队时队列为空
#define FILE_RECORD_QUEUE_DEQUEUE_IS_EMPTY         (3)
//入队时入队项大小不正确
#define FILE_RECORD_QUEUE_ENQUEUE_SIZE_ERROR       (4)
//入队时队满
#define FILE_RECORD_QUEUE_ENQUEUE_IS_FULL          (5)
//获取队列时要获取的数量大于队列已有的数量
#define FILE_RECORD_QUEUE_GET_SIZE_TOO_LARGE       (6)
//文件操作seek错误
#define FILE_RECORD_QUEUE_FILE_SEEK_ERROR          (7)
//文件操作read/write错误
#define FILE_RECORD_QUEUE_FILE_RW_ERROR            (8)
//文件操作修改指针
#define FILE_RECORD_QUEUE_FILE_MODIFY_PTR_ERROR    (9)



/*
 *
 * 等长度的文件队列实现，即队列中的所有内容长度都是固定的例如int、long，char*等
 * 无法存储变长项，例如string等类型
 *
 *
 */
class FileRecordQueue : public QObject
{
    Q_OBJECT
public:
    QString queueName();
    //打开文件
    int open();
    //关闭文件
    void close();
    //获取队列大小
    qint64 size();
    //获取对头元素并删除
    QByteArray dequeue(int* error = nullptr);
    //从队头获取N个数据，不删除
    int get(QList<QByteArray>& list,int* error = nullptr,int size = 0);
    int get(QByteArray& data,int* error = nullptr,int size = 0);
    //从队头删除N个元素，不获取数据
    int removeQueueHeader(int size);
    //入队操作，在队尾添加一条数据
    int enquque(QByteArray&);
    //创建一个文件队列，如果文件存在则读取内容创建队列，如果文件不存在，则创建一个队列文件
    static FileRecordQueue* createFileRecordQueue(QString name,QString filePath,int itemSize,int queueCount,QObject *parent);
    ~FileRecordQueue();
private:
    explicit FileRecordQueue(QString name,QString filePath,QObject *parent = nullptr);
    QString name;//队列名称
    int timeout;
    qint32 en;//入队指针
    qint32 de;//出队指针
    qint32 itemSize;//队列每一项大小
    qint32 queueCount;//队列总大小
    qint64 fileSize;//文件总大小
    QSemaphore* semaphore;//信号量
    QFile* queueFile;//队列文件
signals:
    void onEnQueue(FileRecordQueue*);
public slots:
};

#endif // FILERECORDQUEUE_H
