//
// Created by zy on 17-9-15.
//


#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <stdarg.h>

#include <sstream>      // C++ stream处理
#include <syslog.h>     // 写入系统日志

using namespace std;
#include "Log.h"

#define AGENT_LOG_TMPBUF_SIZE 512                     /* 调试信息每一条语句的长度,超过该长度性能会下降 */

// 预留未来扩充
#define AGENT_LOG_MODULE_LOCK(uiModule, uiLogType) AGENT_OK
#define AGENT_LOG_MODULE_UNLOCK(uiModule, uiLogType) AGENT_OK

// Log模块控制, 未来扩展成类.
typedef struct tagLogConfig
{

    /* 日志是否记录到syslog */
    // syslog是Linux系统中标准日志处理模块, 所有系统日志都可以提交给syslog.
    // syslog的不同priority和facility会写入不同的日志文件, 由/etc/syslog-ng/syslog-ng.conf 文件控制
    // sles 11.3 默认情况下会将ServerAntAgent日志写入/var/log/messages, warning/error日志会额外写入/var/log/warn
    // OS会对syslog的日志进行压缩转储,不用担心日志超限问题.
    // ServerAntAgent启动时会产生几百条info日志, 正常运行时产生的日志较少.
    UINT32   uiLogToSyslog;

    // 如果不使用syslog, 可以将ServerAntAgent日志写入单独的日志文件
    // 由于非root用户无法将日志直接写入/var/log目录, 也无法配置日志转储,所以日志文件会存放到可执行程序的当前目录.
    string strLogFileNameNormal;    // 记录所有日志
    string strLogFileNameWarning;   // 记录warning和error日志
    string strLogFileNameError;     // 记录error日志
    string strLogFileNameLossPacket;     // 记录丢包日志
    string strLogFileNameLatency;     // 记录延时日志

    /* 日志是否打印到终端 */
    UINT32   uiLogToTty;
    UINT32   uiDebug;      /* 值分别为0,1,2,4  */

} LogConfig_S;

static LogConfig_S g_stLogConfig =
        {
                AGENT_FALSE,
                "./logs/ServerAntAgent.log",
                "./logs/ServerAntAgentWarning.log",
                "./logs/ServerAntAgentError.log",
                "./logs/ServerAntAgentLossPacket.log",
                "./logs/ServerAntAgentLatency.log",
                AGENT_FALSE,
                4
        };

#define AGENT_LOG_ERROR_PRINT(...) \
    do  \
    {   \
        INT32 _iRet_ = 0;   \
            _iRet_ = printf(__VA_ARGS__);    \
            if (0 > _iRet_)    \
            {   \
                (void)printf("Printf Error[%d]",_iRet_);\
                return AGENT_E_ERROR;   \
            }   \
    }while(0)

#define AGENT_LOG_CHECK_PARAM(ulModule, ulLogType)   \
    do  \
    {   \
        if ((ulModule) >= AGENT_MODULE_MAX)  \
        {   \
            AGENT_LOG_ERROR_PRINT("[%s]: Invalid Module Number:Mod[%u],Log[%u]\n", __FUNCTION__, (ulModule), (ulLogType));   \
            return AGENT_E_PARA;  \
        }   \
        if ((ulLogType) >= AGENT_LOG_TYPE_MAX)   \
        {   \
            AGENT_LOG_ERROR_PRINT("[%s]: Invalid LogType:Mod[%u],Log[%u]\n", __FUNCTION__, (ulModule), (ulLogType)); \
            return AGENT_E_PARA;  \
        }   \
    }while(0)

// 控制日志是否打印到终端, 日志记录到系统日志还是额外的日志路径
INT32 SetNewLogMode(AgentLogMode_E eNewLogMode)
{
    // 普通启动
    if(AGENT_LOG_MODE_NORMAL == eNewLogMode)
    {
        // 日志记录到文件
        g_stLogConfig.uiLogToSyslog = AGENT_FALSE;
        // 日志打印到标准输出
        g_stLogConfig.uiLogToTty    = AGENT_TRUE;
    }
        // 守护进程模式
    else if(AGENT_LOG_MODE_DAEMON == eNewLogMode)
    {
        // 日志记录到系统日志
        //g_stLogConfig.uiLogToSyslog = AGENT_TRUE;
        // 终端不希望日志记录到系统日志, 单独处理
        g_stLogConfig.uiLogToSyslog = AGENT_FALSE;

        // 日志不打印到标准输出
        g_stLogConfig.uiLogToTty    = AGENT_FALSE;
    }
    else
    {
        AGENT_LOG_ERROR_PRINT("[%s]: Invalid LogMode:[%u]\n", __FUNCTION__, eNewLogMode);
        return AGENT_E_PARA;
    }
    return AGENT_OK;
}

// 控制日志记录目录, 当日志不记录到系统日志时生效
INT32 SetNewLogDir(string strNewDirPath)
{
    // 当前日志记录到系统日志, 修改日志目录不生效
    if(AGENT_TRUE == g_stLogConfig.uiLogToSyslog)
    {
        AGENT_LOG_ERROR_PRINT("[%s]: Set log dir to [%s] when enable LogToSyslog\n", __FUNCTION__, strNewDirPath.c_str());
    }

    g_stLogConfig.strLogFileNameNormal = strNewDirPath + "/ServerAntAgent.log";
    g_stLogConfig.strLogFileNameWarning= strNewDirPath + "/ServerAntAgentWarning.log";
    g_stLogConfig.strLogFileNameError  = strNewDirPath + "/ServerAntAgentError.log";
    g_stLogConfig.strLogFileNameLossPacket  = strNewDirPath + "/ServerAntAgentLossPacket.log";
    g_stLogConfig.strLogFileNameLatency  = strNewDirPath + "/ServerAntAgentLatency.log";

    return AGENT_OK;
}

// 控制日志记录目录, 当日志不记录到系统日志时生效
string GetLossLogFilePath()
{
    return g_stLogConfig.strLogFileNameLossPacket;
}

string GetLatencyLogFilePath()
{
    return g_stLogConfig.strLogFileNameLatency;
}

// 获取当前时间, 用于记录日志
void GetPrintTime(char *timestr)
{
    struct timeval tv;
    time_t t;
    struct tm *tm;

    gettimeofday(&tv, NULL);
    t = (time_t)tv.tv_sec;
    tm = localtime(&t);
    if(NULL == tm)
    {
        return;
    }

    sprintf((char *)timestr, "%04u-%02u-%02u %02u:%02u:%02u",
            tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour,
            tm->tm_min, tm->tm_sec);

    /*
        sprintf((char *)timestr, "[%04u-%02u-%02u %02u:%02u:%02u.%03u]",
                tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour,
                tm->tm_min, tm->tm_sec, (UINT32)tv.tv_usec/1000);
    */

}


// 对字符串格式进行处理,确保日志文件中格式统一, ulBufSize为buffer总大小, 包括结束符
void AgentLogPreParser(char * pcStr, UINT32 ulBufSize)
{
    UINT32 uiStrLen = 0;
    UINT32 uiPreStrState = 0;

    /* 跳过字符串开头的空格 */
    for (uiStrLen = 0; uiStrLen < (ulBufSize - 1); uiStrLen++)
    {
        if (' '  != pcStr[uiStrLen])
        {
            break;
        }
    }

    /*
       将用户日志字符串开头的特殊符号换成空格,不处理[%s]中的特殊符号
       不处理结束符
    */
    for (; uiStrLen < (ulBufSize - 1); uiStrLen++)
    {
        /* 跳过日志模块添加的函数名等信息 */
        if ('[' == pcStr[uiStrLen])
        {
            uiPreStrState = AGENT_TRUE;
            continue;
        }
        if (']' == pcStr[uiStrLen])
        {
            uiPreStrState = AGENT_FALSE;
            continue;
        }
        if (AGENT_TRUE == uiPreStrState)
        {
            continue;
        }
        /* 处理用户日志开头的换行符等特殊字符串 */
        if (   ('\r' == pcStr[uiStrLen])
               || ('\n' == pcStr[uiStrLen])
               || (' '  == pcStr[uiStrLen]))
        {
            pcStr[uiStrLen] = ' ';
        }
        else
        {
            break;
        }
    }

    /* 清除结尾的换行符, 换行符后面统一添加 */
    uiStrLen = strlen(pcStr);
    if ('\n' == pcStr[uiStrLen - 1])
    {
        pcStr[uiStrLen - 1] = ' ';
    }

    return;
}

INT32 WriteToLogFile(const char *pcFileName, const char *pcMsg )
{
    // 写入单独的日志文件
    FILE  * pstFile         = NULL;
    UINT32 uiLength   = 0;
    UINT32 uiStrLen   = 0;

    uiStrLen = sal_strlen(pcMsg);

    pstFile = fopen(pcFileName, "a+");
    if(NULL == pstFile)
    {
        AGENT_LOG_ERROR_PRINT("[%s][%u]: Log can't open file[%s]: %s [%d]\n", __FUNCTION__,  __LINE__, pcFileName, strerror(errno), errno);
        return AGENT_E_ERROR;
    }
    /* 将log写入文件 */
    uiLength = fwrite(pcMsg, sizeof(char), uiStrLen, pstFile);
    fflush(pstFile);
    fclose(pstFile);

    return AGENT_OK;
}

INT32 AgentLogSaveToFile(UINT32 ulLogType, const char *pcMsg )
{
    if (AGENT_TRUE == g_stLogConfig.uiLogToSyslog)
    {
        // 写入syslog
        INT32 iPriority = LOG_INFO;
        switch (ulLogType)
        {
            case AGENT_LOG_TYPE_INFO:
            case AGENT_LOG_TYPE_LOSS_PACKET:
            case AGENT_LOG_TYPE_LATENCY:
                iPriority = LOG_INFO;
                break;

            case AGENT_LOG_TYPE_WARNING:
                iPriority = LOG_WARNING;
                break;

            case AGENT_LOG_TYPE_ERROR:
                iPriority = LOG_ERR;
                break;

            default :
                AGENT_LOG_ERROR_PRINT("[%s][%u]: Log can't support this logtype[%u] \n", __FUNCTION__,  __LINE__, ulLogType);
                return AGENT_E_PARA;
        }

        openlog("ServerAntAgent", LOG_CONS|LOG_PID, LOG_DAEMON);
        syslog(iPriority, pcMsg);
        closelog();
    }
    else
    {
        // 写入单独的日志文件
        const char *pcFileName = NULL;

        switch (ulLogType)
        {
            case AGENT_LOG_TYPE_INFO:
                pcFileName = g_stLogConfig.strLogFileNameNormal.c_str();
                break;

            case AGENT_LOG_TYPE_WARNING:
                pcFileName = g_stLogConfig.strLogFileNameWarning.c_str();
                break;

            case AGENT_LOG_TYPE_ERROR:
                pcFileName = g_stLogConfig.strLogFileNameError.c_str();
                break;

            case AGENT_LOG_TYPE_LOSS_PACKET:
                pcFileName = g_stLogConfig.strLogFileNameLossPacket.c_str();
                break;

            case AGENT_LOG_TYPE_LATENCY:
                pcFileName = g_stLogConfig.strLogFileNameLatency.c_str();
                break;

            default :
                AGENT_LOG_ERROR_PRINT("[%s][%u]: Log can't support this logtype[%u] \n", __FUNCTION__,  __LINE__, ulLogType);
                return AGENT_E_PARA;
        }

        WriteToLogFile(pcFileName, pcMsg);

    }
}

INT32 AgentLogPrintf(AgentModule_E eModule, AgentLogType_E eLogType, const char *szFormat, ...)
{

    va_list         arg;                                        // 用于处理变长参数
    char            acStackLogBuffer[AGENT_LOG_TMPBUF_SIZE];    // 优先使用栈中的buffer, 小而快
    char *          pacHeapLogBuffer = NULL;                    // 当输入字符串长度超过AGENT_LOG_TMPBUF_SIZE时, 从堆中申请空间
    UINT32    uiStrLen = 0;                               // 输入字符串长度
    stringstream    ssLogBuffer;                                // 用于拼接字符串
    char            acCurTime[32]   = {0};                      // 缓存时间戳
    INT32       iRet = AGENT_E_NOT_FOUND;

    /* 入参检查,发现错误后,记录异常,返回*/
    AGENT_LOG_CHECK_PARAM(eModule, eLogType);

    if (NULL == szFormat)
    {
        AGENT_LOG_ERROR_PRINT("[%s]:Module[%u] Log[%u] NULL Input\n", __FUNCTION__, eModule, eLogType);
        return AGENT_E_PARA;
    }

    switch (g_stLogConfig.uiDebug)
    {
        case 0:
            break;

        case 1:
            if (AGENT_LOG_TYPE_ERROR == eLogType)
            {
                iRet  = AGENT_OK;
            }
            break;

        case 2:
            if (AGENT_LOG_TYPE_WARNING == eLogType || AGENT_LOG_TYPE_ERROR == eLogType)
            {
                iRet  = AGENT_OK;
            }
            break;

        default :
            iRet =  AGENT_OK;
            break;
    }

    if (iRet !=  AGENT_OK)
    {
        return iRet;
    }

    /* 生成时间标签 */
    sal_memset(acStackLogBuffer, 0, sizeof(acStackLogBuffer));
    GetPrintTime(acCurTime);

    // 尝试将输入字符串导入acStackLogBuffer中, 若被截断则重新申请内存.
    va_start(arg, szFormat);
    uiStrLen = vsnprintf(acStackLogBuffer, sizeof(acStackLogBuffer), szFormat, arg);
    va_end(arg);

    // 输入字符串太长被截断, 重新申请内存
    va_start(arg, szFormat);
    if (AGENT_LOG_TMPBUF_SIZE <= uiStrLen)
    {
        // 不直接使用string类型是因为string无法识别printf类型的通配符,如%s,%d等.
        pacHeapLogBuffer = new char[uiStrLen + 2];
        // 内存申请成功, 使用新内存缓存日志信息
        if (NULL != pacHeapLogBuffer)
        {
            sal_memset(pacHeapLogBuffer, 0, (uiStrLen + 2));
            uiStrLen = vsnprintf(pacHeapLogBuffer, (uiStrLen + 1), szFormat, arg);
        }
        else
        {
            printf("No enough heap memory for log, msg will be truncated");
        }
    }

    va_end(arg);

    if (NULL == pacHeapLogBuffer)
    {
        /* 对用户字符串中的换行符统一进行处理,处理换行符 */
        AgentLogPreParser(acStackLogBuffer, sizeof(acStackLogBuffer));

        if (AGENT_MODULE_SAVE_REPORTDATA != eModule)
        {
            /* 加入时间戳 */
            ssLogBuffer << acCurTime << " " << acStackLogBuffer << endl;
        }
        else
        {
            ssLogBuffer << acStackLogBuffer << endl;
        }
    }
    else
    {
        /* 对用户字符串中的换行符统一进行处理,处理换行符 */
        AgentLogPreParser(pacHeapLogBuffer, uiStrLen + 2);

        if (AGENT_MODULE_SAVE_REPORTDATA != eModule)
        {
            /* 加入时间戳 */
            ssLogBuffer << acCurTime << " " << pacHeapLogBuffer << endl;
        }
        else
        {
            /* 保存丢包信息不需要时间戳信息 */
            ssLogBuffer << pacHeapLogBuffer << endl;
        }

        /* 释放内存 */
        delete [] pacHeapLogBuffer;
        pacHeapLogBuffer = NULL;
    }

    if (AGENT_TRUE == g_stLogConfig.uiLogToTty)
        printf(ssLogBuffer.str().c_str());
    AgentLogSaveToFile(eLogType, ssLogBuffer.str().c_str());

    return AGENT_OK;
}

