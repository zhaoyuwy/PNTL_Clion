//
// Created by zy on 17-9-15.
//
#include "FileNotifier.h"
#include "AgentCommon.h"
#include "GetLocalCfg.h"
#include "Log.h"

FileNotifier_C::FileNotifier_C()
{
    notifierId = -1;
    wd = -1;
    lastAction = 0;
}

FileNotifier_C::~FileNotifier_C()
{
    INT32 iRet = inotify_rm_watch(notifierId, wd);
    if (0 > iRet)
    {
        FILE_NOTIFIER_ERROR("Remove watch fail[%d]", iRet);
    }

    if (-1 != notifierId)
    {
        iRet = close(notifierId);
        if (0 > iRet)
        {
            FILE_NOTIFIER_ERROR("close watch fail[%d]", iRet);
        }
    }
}

INT32 FileNotifier_C::Init(FlowManager_C* pcFlowManager)
{
    manager = pcFlowManager;
    notifierId = inotify_init();
    if (0 > notifierId)
    {
        FILE_NOTIFIER_ERROR("Create a file notifier fail[%d]", notifierId);
        return AGENT_E_MEMORY;
    }

    wd = inotify_add_watch(notifierId, filePath.c_str(), IN_MODIFY);
    if (0 > wd)
    {
        FILE_NOTIFIER_ERROR("Create a watch Item fail[%d]", wd);
        return AGENT_E_ERROR;
    }
    INT32 iRet = StartThread();
    if(iRet)
    {
        FILE_NOTIFIER_ERROR("StartFileNotifierThread failed[%d]", iRet);
        return iRet;
    }
    return AGENT_OK;
}

INT32 FileNotifier_C::HandleEvent(struct inotify_event * event)
{
    if (event->mask & IN_MODIFY)
    {
        HandleProbePeriod();
    }
    else if (event->mask & IN_IGNORED)
    {
        wd = inotify_add_watch(notifierId, filePath.c_str(), IN_MODIFY);
        if (0 > wd)
        {
            FILE_NOTIFIER_ERROR("Create a watch Item fail[%d]", wd);
            return AGENT_E_ERROR;
        }
        HandleProbePeriod();
    }
}

INT32 FileNotifier_C::PreStopHandler()
{
    return 0;
}

INT32 FileNotifier_C::PreStartHandler()
{
    return 0;
}

INT32 FileNotifier_C::ThreadHandler()
{
    INT32 sizeRead = 0;
    CHAR* pBuf;
    while(GetCurrentInterval())
    {
        sizeRead = read(notifierId, buf, BUF_LEN);
        if (0 > sizeRead)
        {
            continue;
        }
        for (pBuf = buf; pBuf < buf + sizeRead;)
        {
            event = (struct inotify_event *) pBuf;
            HandleEvent(event);
            pBuf +=sizeof(struct inotify_event) + event->len;
        }
    }
}

void FileNotifier_C::HandleProbePeriod()
{
    UINT32 probePeriod = GetProbePeriod(manager);
    if (0 > probePeriod || 120 < probePeriod)
    {
        FILE_NOTIFIER_ERROR("Parse local config file error, probePeriod is [%u], return.", probePeriod);
        return ;
    }
    else if (0 == probePeriod)
    {
        FILE_NOTIFIER_INFO("Probe_period is [%u], will stop flowmanger.", probePeriod);
        manager->FlowManagerAction(STOP_AGENT);
        lastAction = 1;
    }
    else
    {
        if (lastAction)
        {
            FILE_NOTIFIER_INFO("Probe_period is [%u], will start flowmanger.", probePeriod);
            manager->FlowManagerAction(START_AGENT);
            lastAction = 0;
        }
        SHOULD_REFRESH_CONF = 1;
    }
}