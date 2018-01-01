#ifndef PROCESSMANAGER_H
#define PROCESSMANAGER_H

#include <QObject>

typedef struct  {
    const char* processName;
    bool isClose;
} ProcessFilter;

class ProcessManager : public QObject
{
    Q_OBJECT
public:
    explicit ProcessManager(QObject *parent = nullptr);
    //QStringList getMacAddress();
    QStringList getProcesses();
    void checkProcessIsClose(QList<ProcessFilter*>& filter);
signals:

public slots:
};

#endif // PROCESSMANAGER_H
