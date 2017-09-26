//
// Created by zy on 17-9-15.
//

#ifndef PNTL_AGENT_THREADCLASS_H
#define PNTL_AGENT_THREADCLASS_H


#include <pthread.h>
#include "Sal.h"

typedef struct tagThreadControl {
    UINT32 uiThreadState;            // 当前Thread状态, 由Thread自行刷新.
    UINT32 uiThreadInterval;         // Thread运行间隔, 0表示退出, 非0表示运行间隔. (-1)表示不释放CPU, 全速运行.
    UINT32 uiThreadDefaultInterval;  // Thread默认时间间隔.启动任务时uiThreadInterval的默认值. 单位us, 默认100ms.
} ThreadControl_S;

typedef void (*Func)();

// Thread类定义
class ThreadClass_C {
private:
    UINT32 uiThreadUpdateInterval;  // 单位us. Thread状态机刷新时间.StopCallBack()下发后,最长ThreadUpdateInterval us内
    // ThreadHandler()应该返回, 否则Thread会被强制终止.
    UINT32 uiThreadState;           // Thread状态机
    pthread_t ThreadFd;                     // Thread句柄.

public:
    ThreadClass_C();                        // 构造函数, 填充默认值.
    ~ThreadClass_C();                       // 析构函数, 释放必要资源.

    INT32 StopThread();                       // 停止任务.
    INT32 StartThread();                      // 启动任务.

    INT32 SetNewInterval(UINT32 uiNewInterval);     // 设定新定时器间隔
    UINT32 GetCurrentInterval();                  // 查询当前定时器间隔

    INT32 ThreadUpdateState(UINT32 uiNewState);     // 刷新任务状态机

    virtual INT32 ThreadHandler();                        // 任务主处理函数
    virtual INT32 PreStopHandler();                       // StopThread触发, 通知ThreadHandler主动退出.
    virtual INT32 PreStartHandler();                      // StartThread触发, 通知ThreadHandler即将被调用.

};


#endif //PNTL_AGENT_THREADCLASS_H
