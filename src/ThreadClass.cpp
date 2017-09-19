//
// Created by zy on 17-9-15.
//
#include <errno.h>

using namespace std;

#include "Log.h"
#include "ThreadClass.h"

enum
{
    THREAD_STATE_STOPED  = 0,  // 任务未启动或已经停止
    THREAD_STATE_WORKING,      // 任务正在工作
    THREAD_STATE_MAX
};

// 终止任务时, 等待任务主导退出的尝试次数.
#define THREAD_STOP_COUNTER             10
// 默认周期,100ms.
#define THREAD_DEFAULT_UPDATE_INTERVAL  100000
// 最小周期10ms, 一个tick
#define THREAD_UPDATE_MIN               10000


void * ThreadFun(void * p)
{
    INT32 iRet = AGENT_OK;
    ThreadClass_C * pcThread =  (ThreadClass_C *)p;    // 管理本任务的对象.

    pcThread->ThreadUpdateState(THREAD_STATE_WORKING);
    if(iRet)
    {
        THREAD_CLASS_ERROR("Thread Update State To WORKING failed[%d]", iRet);
        pthread_exit(NULL);
    }

    iRet = pcThread->ThreadHandler();
    if(iRet)
    {
        THREAD_CLASS_WARNING("Thread Handler Return Faile[%d]", iRet);
    }

    iRet = pcThread->ThreadUpdateState(THREAD_STATE_STOPED);
    if(iRet)
    {
        THREAD_CLASS_WARNING("Thread Update State To STOP failed[%d]", iRet);
    }
    THREAD_CLASS_INFO("Thread exiting.");
}


// 构造函数, 填充默认值.
ThreadClass_C::ThreadClass_C()
{
    uiThreadUpdateInterval = THREAD_DEFAULT_UPDATE_INTERVAL; // 100ms
    uiThreadState = THREAD_STATE_WORKING;
    ThreadFd = 0;
}

// 析构函数, 释放必要资源.
ThreadClass_C::~ThreadClass_C()
{
    StopThread();           // 停止任务
}


// 设定任务间隔
INT32 ThreadClass_C::SetNewInterval(UINT32 uiNewInterval)
{
    INT32 iRet = AGENT_OK;
    uiThreadUpdateInterval = uiNewInterval;
    return iRet;
}

// 查询当前任务间隔
UINT32 ThreadClass_C::GetCurrentInterval()
{
    return uiThreadUpdateInterval;
}

// 启动任务.
INT32 ThreadClass_C::StartThread()
{
    INT32 iRet = AGENT_OK;

    if (ThreadFd) // 先停止任务.
    {
        StopThread();
    }

    iRet = ThreadUpdateState(THREAD_STATE_STOPED); // 恢复任务默认状态
    if(iRet)
    {
        THREAD_CLASS_ERROR("Thread Update State failed[%d]", iRet);
        ThreadFd = 0;
        return iRet;
    }

    iRet = PreStartHandler();
    if(iRet)
    {
        THREAD_CLASS_ERROR("Pre Start Handler failed[%d]", iRet);
        ThreadFd = 0;
        return AGENT_E_HANDLER;
    }

    iRet = pthread_create(&ThreadFd, NULL, ThreadFun, this);  //启动任务
    if(iRet)    // 任务启动失败
    {
        THREAD_CLASS_ERROR("Create Thread failed[%d]: %s [%d]", iRet, strerror(errno), errno);
        ThreadFd = 0;
        return AGENT_E_THREAD;
    }
    return iRet;
}

// 停止任务.
INT32 ThreadClass_C::StopThread()
{
    UINT32 uiInterval = GetCurrentInterval() / 2 + THREAD_UPDATE_MIN;  // 计算检查周期
    UINT32 uiStopCounter = THREAD_STOP_COUNTER;
    INT32 iRet = AGENT_OK;

    if (ThreadFd)
    {
        THREAD_CLASS_INFO("Stop Thread. Waiting handler's exit. Check Every [%d]us", uiInterval);

        iRet = PreStopHandler();// 通知Handler主动退出.
        if(iRet)
        {
            THREAD_CLASS_WARNING("Pre Stop Handler failed[%d]", iRet);
        }

        do
        {
            sal_usleep(uiInterval);
            uiStopCounter --;
        }
        while ( uiThreadState && uiStopCounter);      // 等待任务主动停止

        if (0 == uiStopCounter)  //任务没有主动停止, 尝试强制退出.
        {
            THREAD_CLASS_WARNING("Stop Thread failed after [%d]us, Force Stop ...", uiInterval * THREAD_STOP_COUNTER);
            iRet = pthread_cancel(ThreadFd);
            if (iRet)
            {
                THREAD_CLASS_ERROR("Force Stop Thread failed[%d]: %s [%d]", iRet, strerror(errno), errno);
                iRet = AGENT_E_ERROR;
            }

            iRet = ThreadUpdateState(THREAD_STATE_STOPED); // 恢复任务默认状态
            if(iRet)
            {
                THREAD_CLASS_ERROR("Thread Update State failed[%d]", iRet);
                ThreadFd = 0;
            }
            THREAD_CLASS_WARNING("Force Stop Thread Sucess");
        }
        else
        {
            THREAD_CLASS_INFO("Stop Thread Sucess. Counter cost [%d]", THREAD_STOP_COUNTER - uiStopCounter);
        }
    }
    ThreadFd = 0;
    return iRet;
}

// 刷新任务状态
INT32 ThreadClass_C::ThreadUpdateState(UINT32 uiNewState)
{
    if (THREAD_STATE_MAX <= uiNewState)
    {
        THREAD_CLASS_ERROR("Thread update to State [%d] failed",
                           uiNewState);
        return AGENT_E_PARA;
    }
    uiThreadState = uiNewState;

    return AGENT_OK;
}

// Thread回调函数.
// PreStopHandler()执行后, ThreadHandler()需要在GetCurrentInterval() us内主动退出.
INT32 ThreadClass_C::ThreadHandler()
{
    while (GetCurrentInterval())
    {
        THREAD_CLASS_WARNING("Thread Handler use default func");
        sal_usleep(GetCurrentInterval());
    }
    return AGENT_OK;
}

// Thread即将启动, 通知ThreadHandler做好准备.
INT32 ThreadClass_C::PreStartHandler()
{
    THREAD_CLASS_WARNING("Pre Start Handler use default func");
}

// Thread即将停止, 通知ThreadHandler主动退出.
INT32 ThreadClass_C::PreStopHandler()
{
    THREAD_CLASS_WARNING("Pre Stop Handler use default func");
    SetNewInterval(0);
}
