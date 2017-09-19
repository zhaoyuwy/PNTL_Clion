//
// Created by zy on 17-9-15.
//

#ifndef PNTL_AGENT_LOG_H
#define PNTL_AGENT_LOG_H



#include "Sal.h"
#include "AgentCommon.h"

typedef enum tagAgentModule
{
    AGENT_MODULE_SAL  = 1,          // 系统适配功能日志
    AGENT_MODULE_TIMER,             // TIMER 适配模块
    AGENT_MODULE_INIT,              // 初始化功能,main函数等
    AGENT_MODULE_COMMON,            // 公共模块
    AGENT_MODULE_THREAD_CLASS,      // THREAD_CLASS 类
    AGENT_MODULE_HTTP_DAEMON,       // HTTP_DAEMON 类
    AGENT_MODULE_KAFKA_CLIENT,      // HTTP_DAEMON 类
    AGENT_MODULE_AGENT_CFG,         // ServerAntAgentCfg 类
    AGENT_MODULE_DETECT_WORKER,     // DetectWorker 类
    AGENT_MODULE_FLOW_MANAGER,      // FlowMananger 类
    AGENT_MODULE_MSG_SERVER,        // MessagePlatformServer 类
    AGENT_MODULE_MSG_CLIENT,        // MSG_CLIENT 模块
    AGENT_MODULE_JSON_PARSER,       // JSON_PARSER 模块(AgentJsonAPI)
    AGENT_MODULE_SAVE_REPORTDATA,
    AGENT_MODULE_FILE_NOTIFIER,
    AGENT_MODULE_MAX

} AgentModule_E;

typedef enum tagAgentLogType
{
    AGENT_LOG_TYPE_INFO  = 1,   // 提示
    AGENT_LOG_TYPE_WARNING,     // 告警
    AGENT_LOG_TYPE_ERROR,       // 错误
    AGENT_LOG_TYPE_LOSS_PACKET,       // 丢包
    AGENT_LOG_TYPE_LATENCY,       // 延时
    AGENT_LOG_TYPE_MAX
} AgentLogType_E;

typedef enum  tagAgentLogMode
{
    AGENT_LOG_MODE_NORMAL = 0,   // 日志直接打印到终端,且在调用程序的当前目录创建日志文件.
    AGENT_LOG_MODE_DAEMON,       // 日志不打印到终端,直接记录到syslog.
    AGENT_LOG_MODE_MAX
} AgentLogMode_E;

extern INT32 SetNewLogMode(AgentLogMode_E eNewLogMode);
extern INT32 SetNewLogDir(string strNewDirPath);
extern void GetPrintTime(char *timestr);
extern INT32 AgentLogPrintf(AgentModule_E eModule, AgentLogType_E eLogType, const CHAR *szFormat, ...);
extern string GetLossLogFilePath();
extern string GetLatencyLogFilePath();

//__PRETTY_FUNCTION__
#if 1
#define MODULE_LOSS(module, format, ...)                                        \
do                                                                              \
{                                                                               \
    AgentLogPrintf(AGENT_MODULE_ ## module, AGENT_LOG_TYPE_LOSS_PACKET, format, ##__VA_ARGS__);    \
} while(0)

#define MODULE_LATENCY(module, format, ...)                                        \
do                                                                              \
{                                                                               \
    AgentLogPrintf(AGENT_MODULE_ ## module, AGENT_LOG_TYPE_LATENCY,  format, ##__VA_ARGS__);    \
} while(0)

#define MODULE_INFO(module, format, ...)                                        \
do                                                                              \
{                                                                               \
    AgentLogPrintf(AGENT_MODULE_ ## module, AGENT_LOG_TYPE_INFO,                \
            "[Info]    " #module " [%s][%d] " format, __FUNCTION__, __LINE__, ##__VA_ARGS__);    \
} while(0)

#define MODULE_WARNING(module, format, ...)                                     \
do                                                                              \
{                                                                               \
    AgentLogPrintf(AGENT_MODULE_ ## module, AGENT_LOG_TYPE_WARNING,             \
            "[Warning] " #module " [%s][%d] " format, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
} while(0)

#define MODULE_ERROR(module, format, ...)                                       \
do                                                                              \
{                                                                               \
    AgentLogPrintf(AGENT_MODULE_ ## module, AGENT_LOG_TYPE_ERROR,               \
            "[Error]   " #module " [%s][%d] " format, __FUNCTION__, __LINE__, ##__VA_ARGS__);   \
} while(0)
#else
#define MODULE_INFO(module, format, ...)                                        \
do                                                                              \
{                                                                               \
    AgentLogPrintf(AGENT_MODULE_ ## module, AGENT_LOG_TYPE_INFO,                \
            "[Info]    "#module" [%s][%d] "format, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__);    \
} while(0)

#define MODULE_WARNING(module, format, ...)                                     \
do                                                                              \
{                                                                               \
    AgentLogPrintf(AGENT_MODULE_ ## module, AGENT_LOG_TYPE_WARNING,             \
            "[Warning] "#module" [%s][%d] "format, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__); \
} while(0)

#define MODULE_ERROR(module, format, ...)                                       \
do                                                                              \
{                                                                               \
    AgentLogPrintf(AGENT_MODULE_ ## module, AGENT_LOG_TYPE_ERROR,               \
            "[Error]   "#module" [%s][%d] "format, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__);   \
} while(0)
#endif
#define SAL_INFO(...)               MODULE_INFO(SAL,   __VA_ARGS__)
#define TIMER_INFO(...)             MODULE_INFO(TIMER,   __VA_ARGS__)
#define INIT_INFO(...)              MODULE_INFO(INIT,   __VA_ARGS__)
#define COMMON_INFO(...)            MODULE_INFO(COMMON,   __VA_ARGS__)
#define AGENT_CFG_INFO(...)         MODULE_INFO(AGENT_CFG,   __VA_ARGS__)
#define THREAD_CLASS_INFO(...)      MODULE_INFO(THREAD_CLASS,   __VA_ARGS__)
#define HTTP_DAEMON_INFO(...)       MODULE_INFO(HTTP_DAEMON,   __VA_ARGS__)
#define KAFKA_CLIENT_INFO(...)      MODULE_INFO(KAFKA_CLIENT,   __VA_ARGS__)
#define DETECT_WORKER_INFO(...)     MODULE_INFO(DETECT_WORKER,   __VA_ARGS__)
#define FLOW_MANAGER_INFO(...)      MODULE_INFO(FLOW_MANAGER,   __VA_ARGS__)
#define MSG_SERVER_INFO(...)        MODULE_INFO(MSG_SERVER,   __VA_ARGS__)
#define MSG_CLIENT_INFO(...)        MODULE_INFO(MSG_CLIENT,   __VA_ARGS__)
#define JSON_PARSER_INFO(...)       MODULE_INFO(JSON_PARSER,   __VA_ARGS__)
#define SAVE_LOSS_INFO(...)         MODULE_LOSS(SAVE_REPORTDATA,__VA_ARGS__)
#define SAVE_LATENCY_INFO(...)      MODULE_LATENCY(SAVE_REPORTDATA, __VA_ARGS__)
#define FILE_NOTIFIER_INFO(...)     MODULE_INFO(FILE_NOTIFIER, __VA_ARGS__)

#define SAL_WARNING(...)               MODULE_WARNING(SAL,   __VA_ARGS__)
#define TIMER_WARNING(...)             MODULE_WARNING(TIMER,   __VA_ARGS__)
#define INIT_WARNING(...)              MODULE_WARNING(INIT,   __VA_ARGS__)
#define COMMON_WARNING(...)            MODULE_WARNING(COMMON,   __VA_ARGS__)
#define THREAD_CLASS_WARNING(...)      MODULE_WARNING(THREAD_CLASS,   __VA_ARGS__)
#define HTTP_DAEMON_WARNING(...)       MODULE_WARNING(HTTP_DAEMON,   __VA_ARGS__)
#define KAFKA_CLIENT_WARNING(...)      MODULE_WARNING(KAFKA_CLIENT,   __VA_ARGS__)
#define AGENT_CFG_WARNING(...)         MODULE_WARNING(AGENT_CFG,   __VA_ARGS__)
#define DETECT_WORKER_WARNING(...)     MODULE_WARNING(DETECT_WORKER,   __VA_ARGS__)
#define FLOW_MANAGER_WARNING(...)      MODULE_WARNING(FLOW_MANAGER,   __VA_ARGS__)
#define MSG_SERVER_WARNING(...)        MODULE_WARNING(MSG_SERVER,   __VA_ARGS__)
#define MSG_CLIENT_WARNING(...)        MODULE_WARNING(MSG_CLIENT,   __VA_ARGS__)
#define JSON_PARSER_WARNING(...)       MODULE_WARNING(JSON_PARSER,   __VA_ARGS__)
#define FILE_NOTIFIER_WARNING(...)     MODULE_WARNING(FILE_NOTIFIER, __VA_ARGS__)

#define SAL_ERROR(...)               MODULE_ERROR(SAL,   __VA_ARGS__)
#define TIMER_ERROR(...)             MODULE_ERROR(TIMER,   __VA_ARGS__)
#define INIT_ERROR(...)              MODULE_ERROR(INIT,   __VA_ARGS__)
#define COMMON_ERROR(...)            MODULE_ERROR(COMMON,   __VA_ARGS__)
#define THREAD_CLASS_ERROR(...)      MODULE_ERROR(THREAD_CLASS,   __VA_ARGS__)
#define HTTP_DAEMON_ERROR(...)       MODULE_ERROR(HTTP_DAEMON,   __VA_ARGS__)
#define KAFKA_CLIENT_ERROR(...)      MODULE_ERROR(KAFKA_CLIENT,   __VA_ARGS__)
#define AGENT_CFG_ERROR(...)         MODULE_ERROR(AGENT_CFG,   __VA_ARGS__)
#define DETECT_WORKER_ERROR(...)     MODULE_ERROR(DETECT_WORKER,   __VA_ARGS__)
#define FLOW_MANAGER_ERROR(...)      MODULE_ERROR(FLOW_MANAGER,   __VA_ARGS__)
#define MSG_SERVER_ERROR(...)        MODULE_ERROR(MSG_SERVER,   __VA_ARGS__)
#define MSG_CLIENT_ERROR(...)        MODULE_ERROR(MSG_CLIENT,   __VA_ARGS__)
#define JSON_PARSER_ERROR(...)       MODULE_ERROR(JSON_PARSER,   __VA_ARGS__)
#define FILE_NOTIFIER_ERROR(...)     MODULE_ERROR(FILE_NOTIFIER, __VA_ARGS__)



#endif //PNTL_AGENT_LOG_H
