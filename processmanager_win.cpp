#include "processmanager.h"
#include "qdebug.h"


#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <Psapi.h>


ProcessManager::ProcessManager(QObject *parent) : QObject(parent)
{

}

QStringList ProcessManager::getProcesses()
{
    QStringList result;
    DWORD arrProIds[1024], ByteCnt, ProCnt;

    //获得所有的进程的ID
    if (!EnumProcesses(arrProIds, sizeof(arrProIds), &ByteCnt))
    {
        return result;
    }
    //计算获得的进程ID的数量
    ProCnt = ByteCnt / sizeof(DWORD);
    //输出所有进程的名称
    for (unsigned int i = 0; i < ProCnt; i++)
    {
        if( arrProIds[i] != 0 )
        {
            TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");
            //获得进程句柄
            HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |PROCESS_VM_READ,FALSE, arrProIds[i]);
            //得到进程的名称
            if (hProcess != NULL)
            {
                HMODULE hMod;
                DWORD ByteCnt;
                if(EnumProcessModules(hProcess, &hMod, sizeof(hMod),&ByteCnt))
                {
                    GetModuleBaseName( hProcess, hMod, szProcessName,sizeof(szProcessName)/sizeof(TCHAR));
                    result << QString::fromUtf16((char16_t*)szProcessName);
                }
            }
            CloseHandle(hProcess);
        }
    }
    return result;
}
void ProcessManager::checkProcessIsClose(QList<ProcessFilter*>& filter)
{
    if(filter.size() == 0)
    {
         return;
    }
    DWORD arrProIds[1024], ByteCnt, ProCnt;

    //获得所有的进程的ID
    if (!EnumProcesses(arrProIds, sizeof(arrProIds), &ByteCnt))
    {
        return;
    }
    //计算获得的进程ID的数量
    ProCnt = ByteCnt / sizeof(DWORD);
    for(int i=0 ; i< filter.size() ;i++)
    {
        filter[i]->isClose = true;
    }
    //输出所有进程的名称
    for (unsigned int i = 0; i < ProCnt; i++)
    {
        if( arrProIds[i] != 0 )
        {
            TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");
            //获得进程句柄
            HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |PROCESS_VM_READ,FALSE, arrProIds[i]);
            //得到进程的名称
            if (hProcess != NULL)
            {
                HMODULE hMod;
                DWORD ByteCnt;
                if(EnumProcessModules(hProcess, &hMod, sizeof(hMod),&ByteCnt))
                {
                    GetModuleBaseName( hProcess, hMod, szProcessName,sizeof(szProcessName)/sizeof(TCHAR));
                    QString pn = QString::fromUtf16((char16_t*)szProcessName);
                    for(int j=0 ,len = filter.size(); j < len ; j++)
                    {
                        QString cName(filter[j]->processName);
                        //qDebug() << cName;
                        if(cName.compare(pn,Qt::CaseInsensitive)==0)
                        {
                            filter[j]->isClose = false;
                            break;
                        }
                    }

                }
            }
            CloseHandle(hProcess);
        }
    }
}
