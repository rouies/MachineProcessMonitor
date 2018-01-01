#include "processmanager.h"

#include "qdebug.h"

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/network/IOEthernetInterface.h>
#include <IOKit/network/IONetworkInterface.h>
#include <IOKit/network/IOEthernetController.h>

#include <sys/proc_info.h>
#include <libproc.h>
#include <string.h>
/*
#include <sys/sysctl.h>
#include <sys/types.h>

//获取更多的进程信息
    struct kinfo_proc* processes = NULL;
    size_t procSize = 0;
    int name[4] = {CTL_KERN,KERN_PROC,KERN_PROC_ALL,0};
    int st = sysctl(name,4,NULL,&procSize,NULL,0);
    if(st != -1){
        processes = (kinfo_proc*)malloc(procSize);
        if(processes != NULL){
            bzero(processes,procSize);
            st = sysctl(name,4,processes,&procSize,NULL,0);
            if(st != -1){
                int procCount = procSize / sizeof(struct kinfo_proc);
                for(int i = 0 ; i < procCount ; i++){
                    pid_t pid = processes[i].kp_proc.p_pid;
                    //printf("%d\n",pid);
                    char pathBuffer[PROC_PIDPATHINFO_MAXSIZE];
                    bzero(pathBuffer,PROC_PIDPATHINFO_MAXSIZE);
                    proc_pidpath(pid,pathBuffer,sizeof(pathBuffer));
                    if(strlen(pathBuffer) > 0){
                        printf("%s\n",pathBuffer);
                    }
                }
            }
        }
        free(processes);
    }
*/
/*
static kern_return_t FindEthernetInterfaces(io_iterator_t *matchingServices)
{
    kern_return_t           kernResult;
    CFMutableDictionaryRef  matchingDict;
    CFMutableDictionaryRef  propertyMatchDict;

    matchingDict = IOServiceMatching(kIOEthernetInterfaceClass);

    if (NULL == matchingDict)
    {
        printf("IOServiceMatching returned a NULL dictionary.\n");
    }
    else
    {
        propertyMatchDict = CFDictionaryCreateMutable(kCFAllocatorDefault, 0,
                                                      &kCFTypeDictionaryKeyCallBacks,
                                                      &kCFTypeDictionaryValueCallBacks);
        if (NULL == propertyMatchDict)
        {
            printf("CFDictionaryCreateMutable returned a NULL dictionary.\n");
        }
        else
        {
            CFDictionarySetValue(propertyMatchDict, CFSTR(kIOPrimaryInterface), kCFBooleanTrue);
            CFDictionarySetValue(matchingDict, CFSTR(kIOPropertyMatchKey), propertyMatchDict);
            CFRelease(propertyMatchDict);
        }
    }
    kernResult = IOServiceGetMatchingServices(kIOMasterPortDefault, matchingDict, matchingServices);
    if (KERN_SUCCESS != kernResult)
    {
        printf("IOServiceGetMatchingServices returned 0x%08x\n", kernResult);
    }

    return kernResult;
}

static kern_return_t GetMACAddress(io_iterator_t intfIterator,QStringList& result)
{
    io_object_t     intfService;
    io_object_t     controllerService;
    kern_return_t   kernResult = KERN_FAILURE;

    UInt8 MACAddress[kIOEthernetAddressSize];
    UInt8 bufferSize = kIOEthernetAddressSize;

    while ((intfService = IOIteratorNext(intfIterator)))
    {
        bzero(MACAddress, bufferSize);
        CFTypeRef   MACAddressAsCFData;
        kernResult = IORegistryEntryGetParentEntry(intfService,
                                                   kIOServicePlane,
                                                   &controllerService);

        if (KERN_SUCCESS != kernResult)
        {
            printf("IORegistryEntryGetParentEntry returned 0x%08x\n", kernResult);
        }
        else
        {
            MACAddressAsCFData = IORegistryEntryCreateCFProperty(controllerService,
                                                                 CFSTR(kIOMACAddress),
                                                                 kCFAllocatorDefault,
                                                                 0);
            if (MACAddressAsCFData)
            {
                CFShow(MACAddressAsCFData);
                CFDataGetBytes((CFDataRef)MACAddressAsCFData, CFRangeMake(0, kIOEthernetAddressSize), MACAddress);
                CFRelease(MACAddressAsCFData);
                QString str = QString("%1-%2-%3-%4-%5-%6");
                result << str.arg(MACAddress[0],2,16,QLatin1Char('0'))
                             .arg(MACAddress[1],0,16,QLatin1Char('0'))
                             .arg(MACAddress[2],0,16,QLatin1Char('0'))
                             .arg(MACAddress[3],0,16,QLatin1Char('0'))
                             .arg(MACAddress[4],2,16,QLatin1Char('0'))
                             .arg(MACAddress[5],0,16,QLatin1Char('0'))
                             .toUpper();
            }
            (void) IOObjectRelease(controllerService);
        }
    }
    return kernResult;
}

QStringList ProcessManager::getMacAddress()
{
    QStringList result;
    io_iterator_t iterator;
    kern_return_t res = FindEthernetInterfaces(&iterator);
    if(res == KERN_SUCCESS)
    {
        GetMACAddress(iterator,result);
     }
     return result;
}
*/

ProcessManager::ProcessManager(QObject *parent) : QObject(parent)
{

}



QStringList ProcessManager::getProcesses(){
    QStringList result;
    int processCount = proc_listpids(PROC_ALL_PIDS,0,NULL,0);
    pid_t pids[processCount];
    proc_listpids(PROC_ALL_PIDS,0,pids,sizeof(pids));
    for(int i = 0 ; i < processCount ;i++)
    {
        if(pids[i] == 0)
        {
            continue;
        }
        char pathBuffer[PROC_PIDPATHINFO_MAXSIZE];
        bzero(pathBuffer,PROC_PIDPATHINFO_MAXSIZE);
        proc_pidpath(pids[i],pathBuffer,sizeof(pathBuffer));
        if(strlen(pathBuffer) > 0)
        {
            result << QString(pathBuffer);
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
    int processCount = proc_listpids(PROC_ALL_PIDS,0,NULL,0);
    pid_t pids[processCount];
    proc_listpids(PROC_ALL_PIDS,0,pids,sizeof(pids));
    for(int j = 0 , len = filter.size() ; j < len ; j++ )
    {
        filter[j]->isClose = true;
    }
    for(int i = 0 ; i < processCount ;i++)
    {
        if(pids[i] == 0)
        {
            continue;
        }
        char pathBuffer[PROC_PIDPATHINFO_MAXSIZE];
        bzero(pathBuffer,PROC_PIDPATHINFO_MAXSIZE);
        proc_pidpath(pids[i],pathBuffer,sizeof(pathBuffer));
        for(int j = 0 , len = filter.size() ; j < len ; j++ )
        {
            if(strcmp(pathBuffer,filter.at(j)->processName) == 0)
            {
                filter[j]->isClose = false;
                break;
            }
        }
    }
}
