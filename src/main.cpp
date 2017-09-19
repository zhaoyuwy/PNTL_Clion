//
// Created by zy on 17-9-15.
//

#include <stdlib.h>
#include <errno.h>
#include <pthread.h>

using namespace std;
#include "Log.h"
#include "GetLocalCfg.h"
#include "ServerAntAgentCfg.h"
#include "MessagePlatform.h"
#include "FileNotifier.h"
#include "FlowManager.h"
#include "MessagePlatformClient.h"
#include "AgentCommon.h"

void destroyServerCfgObj(ServerAntAgentCfg_C * pcCfg)
{
    if (NULL != pcCfg)
    {
        delete pcCfg;
    }
}

void destroyFlowManagerObj(FlowManager_C * pcFlowManager)
{
    if (NULL != pcFlowManager)
    {
        delete pcFlowManager;
    }
}

void destroyFileNotifier(FileNotifier_C* pcFileNotifier)
{
    if (NULL != pcFileNotifier)
    {
        delete pcFileNotifier;
    }
}

// 启动ServerAntAgent业务
INT32 ServerAntAgent()
{
    INT32 iRet = 0;
    UINT32 uiPort = 0;
    FlowManager_C * pcFlowManager = NULL;

    INIT_INFO("-------- Starting ServerAntAgent Now --------");

    // 创建ServerAntAgentCfg对象, 用于保存agent配置信息
    ServerAntAgentCfg_C * pcCfg = new ServerAntAgentCfg_C;
    if (NULL == pcCfg)
    {
        return AGENT_E_MEMORY;
    }

    // 获取本地配置信息
    INIT_INFO("-------- GetLocalCfg --------");
    iRet = GetLocalCfg(pcCfg);
    if (AGENT_OK != iRet)
    {
        destroyServerCfgObj(pcCfg);
        INIT_ERROR("GetLocalCfg failed [%d]", iRet);
        return iRet;
    }

    pcFlowManager = new FlowManager_C(pcCfg);
    INIT_INFO("--------  Get agentConfig.cfg -------- ");
    iRet = GetLocalAgentConfig(pcFlowManager);
    if (AGENT_OK != iRet)
    {
        destroyFlowManagerObj(pcFlowManager);
        destroyServerCfgObj(pcCfg);
        INIT_ERROR("GetLocalAgentConfig failed [%d]", iRet);
        return iRet;
    }

    // 启动FlowManager对象
    INIT_INFO("-------- Start FlowManager --------");
    iRet = pcFlowManager->Init(pcCfg);
    if (AGENT_OK != iRet)
    {
        destroyFlowManagerObj(pcFlowManager);
        destroyServerCfgObj(pcCfg);
        INIT_ERROR("FlowManager.init failed [%d]", iRet);
        return iRet;
    }

    FileNotifier_C* pcFileNotifier = new FileNotifier_C;
    iRet = pcFileNotifier -> Init(pcFlowManager);
    if (iRet)
    {
        INIT_ERROR("Init filenotifier error[%d].", iRet);
        destroyFileNotifier(pcFileNotifier);
        destroyFlowManagerObj(pcFlowManager);
        destroyServerCfgObj(pcCfg);
        return iRet;
    }


    // 所有对象已经启动完成, 开始工作.
    INIT_INFO("-------- Starting ServerAntAgent Complete --------");

    UINT32 reportCount = 1;
    do
    {
        iRet = ReportAgentIPToServer(pcCfg);
        if (AGENT_OK != iRet)
        {
            INIT_ERROR("Report Agent ip to Server fail[%d]", iRet);
            sleep(5 * reportCount);
            INIT_INFO("Retry to report Agent ip to Server, time [%u]", ++reportCount);
        }
        else
        {
            SHOULD_DETECT_REPORT = 1;
        }
    }
    while (iRet);

    while(1)
    {
        sal_sleep(10);
        // 未来考虑诊断命令行.
    }

    INIT_INFO("-------- Stopping ServerAntAgent Now --------");

    destroyFlowManagerObj(pcFlowManager);
    destroyServerCfgObj(pcCfg);
    destroyFileNotifier(pcFileNotifier);
    INIT_INFO("-------- ServerAntAgent Exit Now --------");

    return AGENT_OK;
}

UINT32 SHOULD_REPORT_IP = 0;

UINT32 SHOULD_DETECT_REPORT = 0;

UINT32 SHOULD_REFRESH_CONF = 0;

// 程序入口, 默认直接启动.
// 不带参数时直接启动
// -d 作为守护进程启动
INT32 main (INT32 argc, char **argv)
{
    INT32 iRet = AGENT_OK;
    // 软件启动模式
    INT32 iStartAsDaemon = AGENT_FALSE;

    pid_t pid;

    SetNewLogMode(AGENT_LOG_MODE_NORMAL);

    // 参数解析
    if ( 2 <= argc)
    {
        string strTemp ;
        for (INT32 iIndex = 1; iIndex < argc; iIndex++ )
        {
            strTemp = argv[iIndex];
            if ( "-d" == strTemp )
            {
                iStartAsDaemon = AGENT_TRUE;
            }
            else
            {
                INIT_ERROR("-------- Unknown arg[%s] --------", strTemp.c_str());
                exit(-1);
            }
        }
    }

    if (iStartAsDaemon)
    {
        // 以守护进程的形式启动
        SetNewLogMode(AGENT_LOG_MODE_DAEMON);
        INIT_INFO("-------- StartAsDaemon --------");

        // 1. 创建子进程
        pid = fork();
        if (0 > pid)
        {
            INIT_ERROR("-------- fork error --------");
            exit(-1);
        }
        else if (0 < pid)
        {
            // 父进程处理, 直接退出
            exit(0);
        }
        else
        {
            // 子进程处理

            // 2. 子进程创建新会话, 不再受父进程会话影响.
            setsid();

            // 3. 改变子进程工作目录为指定目录
            // chdir("/");

            // 4. 重设文件权限掩码(创建文件时的默认权限).
            //umask(0);

            //5. 休眠2s, 等启动脚本配置cgroup等信息.
            sleep(2);

            // 6. 启动守护业务.
            iRet = ServerAntAgent();

            // 7. 业务处理完成,准备退出.

        }
    }
    else
    {
        // 直接启动
        iRet = ServerAntAgent();
    }

    if(iRet)
        exit(-1);
    else
        exit(0);
}
