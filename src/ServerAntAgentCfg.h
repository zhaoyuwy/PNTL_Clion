//
// Created by zy on 17-9-15.
//

#ifndef PNTL_AGENT_SERVERANTAGENTCFG_H
#define PNTL_AGENT_SERVERANTAGENTCFG_H



#include <string>
#include "AgentCommon.h"

/*
UDP 协议需要源端口范围用于申请探测socket, 目的端口用于响应探测报文及填充探测
*/
typedef struct tagServerAntAgentProtocolUDP
{
    UINT32 uiDestPort;               // 探测报文的目的端口, 服务端监听端口, 用于监听探测报文并进行应答.
    UINT32 uiSrcPortMin;             // 客户端探测报文源端口范围Min, 用于发送探测报文并监听应答.
    UINT32 uiSrcPortMax;             // 客户端探测报文源端口范围Max, 用于发送探测报文并监听应答.
} ServerAntAgentProtocolUDP_S;


// ServerAntAgent全局配置信息
class ServerAntAgentCfg_C
{
private:
    /* ServerAnt地址信息,管理通道 */
    UINT32 uiServerIP;                //  ServerAntServer的IP地址, Agent向Server发起查询会话时使用.
    UINT32 uiServerDestPort;          //  ServerAntServer的端口地址, Agent向Server发起查询会话时使用.

    UINT32 uiAgentIP;                 //  本Agent的数据面IP地址, Agent探测IP.
    UINT32 uiMgntIP;                  // 本Agent的管理面IP地址，Server向Agent推送消息时使用.

    /* Agent 全局周期控制 */
    UINT32 uiAgentPollingTimerPeriod; // Agent Polling周期, 单位为us, 默认100ms, 用于设定Agent定时器.
    UINT32 uiAgentReportPeriod;       // Agent向Collector上报周期, 单位Polling周期, 默认3000(300s).
    UINT32 uiAgentQueryPeriod;        // Agent向Server查询周期, 单位Polling周期, 默认30000(300s).

    /* Detect 控制 */
    UINT32 uiAgentDetectPeriod;       // Agent探测其他Agent周期, 单位Polling周期, 默认20(2s).
    UINT32 uiAgentDetectTimeout;      // Agent探测报文超时时间, 单位Polling周期, 默认10(1s).
    UINT32 uiAgentDetectDropThresh;   // Agent探测报文丢包门限, 单位为报文个数, 默认5(即连续5个探测报文超时即认为链接出现丢包).
    UINT32 uiPortCount;               // Agent探测报文的源端口范围
    UINT32 uiDscp;                    // Agent探测报文的Dscp值
    UINT32 uiBigPkgRate;              // Agent探测报文中大包占用的比例
    UINT32 uiMaxDelay;                // 最大时延，低于此值的数据不上报

    /* Detect 协议控制参数 */
    ServerAntAgentProtocolUDP_S stProtocolUDP; // UDP 探测报文全局设定,包括源端口范围及目的端口信息.
    string kafkaIp;
    string kafkaBasicToken;
    string topic;
    UINT32 dropPkgThresh;
    UINT32 bigPkgSize;
    sal_mutex_t AgentCfgLock;               // 互斥锁保护

public:

    ServerAntAgentCfg_C();                  // 类构造函数, 填充默认值.
    ~ServerAntAgentCfg_C();                 // 类析构函数, 释放必要资源.

    void GetServerAddress(UINT32 * puiServerIP,
                          UINT32 * puiServerDestPort);        // 查询ServerAntServer地址信息.
    void SetServerAddress(UINT32 uiNewServerIP,
                          UINT32 uiNewServerDestPort);         // 设置ServerAntServer地址信息, 非0有效.

    void GetAgentAddress(UINT32 * puiAgentIP);         // 查询ServerAntAgent地址信息.
    void SetAgentAddress(UINT32 uiNewAgentIP);          // 设置ServerAntAgent地址信息, 非0有效.

    void GetMgntIP(UINT32* puiMgntIP);
    void SetMgntIP(UINT32 uiNewMgntIP);         // 设定管理口ip

    UINT32 GetPollingTimerPeriod();   // 查询Polling周期
    INT32 SetPollingTimerPeriod(UINT32 uiNewPeriod);  //设置Polling周期, 如跟已有周期不一致则同时刷新定时器

    UINT32 GetDetectPeriod();                         // 查询Detect周期
    void SetDetectPeriod(UINT32 uiNewPeriod);         // 设定Detect周期

    UINT32 GetAgentIP();          // 查询ServerAntAgent地址信息.

    UINT32 GetReportPeriod();                         // 查询Report周期
    void SetReportPeriod(UINT32 uiNewPeriod);         // 设定Report周期

    UINT32 GetQueryPeriod();                         // 查询query周期
    void SetQueryPeriod(UINT32 uiNewPeriod);         // 设定query周期

    UINT32 GetDetectTimeout();                        // 查询Detect报文超时时间
    void SetDetectTimeout(UINT32 uiNewPeriod);         // 设定Detect报文超时时间

    UINT32 GetDetectDropThresh();                         // 查询Detect报文丢包门限
    void SetDetectDropThresh(UINT32 uiNewThresh);         // 设定Detect报文丢包门限

    void GetProtocolUDP(UINT32 * puiSrcPortMin,
                        UINT32 * puiSrcPortMax,
                        UINT32 * puiDestPort);           // 查询UDP探测报文端口范围.
    void SetProtocolUDP(UINT32 uiSrcPortMin,
                        UINT32 uiSrcPortMax,
                        UINT32 uiDestPort);             // 设定UDP探测报文端口范围, 只刷新非0端口

    UINT32 GetPortCount();
    void SetPortCount(UINT32 newPortCount);

    UINT32 getDscp();
    void SetDscp(UINT32 newDscp);

    UINT32 GetBigPkgRate();
    void SetBigPkgRate(UINT32 newRate);

    UINT32 GetMaxDelay();
    void SetMaxDelay(UINT32 newMaxDelay);

    UINT32 GetBigPkgSize();
    void SetBigPkgSize(UINT32 newSize);

    string GetKafkaIp();
    void SetKafkaIp(string newIp);

    string GetKafkaBasicToken();
    void SetKafkaBasicToken(string newIp);

    string GetTopic();

    void SetTopic(string newTopic);
};


#endif //PNTL_AGENT_SERVERANTAGENTCFG_H
