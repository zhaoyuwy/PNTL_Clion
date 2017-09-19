//
// Created by zy on 17-9-15.
//

#ifndef PNTL_AGENT_FLOWMANAGER_H
#define PNTL_AGENT_FLOWMANAGER_H



#include "ThreadClass.h"
#include "DetectWorker.h"
#include "ServerAntAgentCfg.h"


// 每个报文的探测结果.(实时刷新)
typedef struct tagDetectResultPkt
{
    /* 会话管理 */
    UINT32   uiSessionState;      // 会话状态机: 待发送探测报文, 等待应答报文, 已收到应答报文, 报文超时.
    UINT32   uiSequenceNumber;    // 本会话的序列号.

    /* 探测结果 */
    PacketTime_S    stT1;               //时间戳信息, 探测报文从Sender出发时间.
    PacketTime_S    stT2;               //时间戳信息, 探测报文到达Target的时间.
    PacketTime_S    stT3;               //时间戳信息, 应答报文从Target出发的时间.
    PacketTime_S    stT4;               //时间戳信息, 应答报文到达Sender的时间.
} DetectResultPkt_S;

// 整条流的探测结果, 准备上报给Collector.
typedef struct tagDetectResult
{
    INT64    lT1;                        // 时间戳信息, 单位ms, 从Epoch开始, 第一个探测报文从Sender出发时间.(实时刷新)
    INT64    lT2;                        // 时间戳信息, 单位ms, 从Epoch开始, 第一个探测报文到达Target的时间.(实时刷新)
    INT64    lT3;                        // 时延 us, Target平均处理时间(stT3 - stT2), (上报时触发计算刷新)
    INT64    lT4;                        // 时延 us, Sender报文平均往返时间(stT4 - stT1), RTT.(上报时触发计算刷新)
    INT64    lT5;                        // 时间戳信息, 单位ms, 从Epoch开始, 本结果上报时间. (上报时触发计算刷新).

    INT64    lDropNotesCounter;          // 触发DropNotes上报次数 (实时刷新)
    INT64    lPktSentCounter;            // 成功发送的探测报文数 (实时刷新)
    INT64    lPktDropCounter;            // 没有在指定时间内收到应答报文的报文数 (实时刷新)
    INT64    lLatencyMin;                // 所有探测报文RTT的最小值 (上报时触发计算刷新)
    INT64    lLatencyMax;                // 所有探测报文RTT的最大值 (上报时触发计算刷新)
    INT64    lLatency50Percentile;       // 所有探测报文RTT的50%位数(中位数) (上报时触发计算刷新)
    INT64    lLatency99Percentile;       // 所有探测报文RTT的99%位数 (上报时触发计算刷新)
    INT64    lLatencyStandardDeviation;  // 所有探测报文RTT的标准差 (上报时触发计算刷新)
} DetectResult_S;


// AgentFlowTable 支持添加, 不支持删除(会导致index错乱), 可以enable/disable flow.
typedef struct tagAgentFlowTableEntry
{
    /* 流信息6元组 */
    FlowKey_S   stFlowKey;

    UINT32 uiFlowState;   // 当前流状态, 丢包/普通, 追踪/普通, enable/disable.
    // Urgent流探测完成后启动上报并disable, 不能删除, 否则index会错乱.
    // 每次由普通状态切换成丢包状态时触发丢包上报.

    /* 原始探测结果 */
    vector <DetectResultPkt_S> vFlowDetectResultPkt;
    UINT32 uiFlowDropCounter;     // 流当前连续丢包计数, 超过门限后触发丢包上报事件.

    /* 准备上报的探测结果 */
    DetectResult_S stFlowDetectResult;
} AgentFlowTableEntry_S;

typedef struct tagServerFlowKey
{
    UINT32   uiUrgentFlow;        // 流优先级.

    AgentDetectProtocolType_E eProtocol;// 协议类型, 目前支持UDP, 见AgentDetectProtocolType_E.
    UINT32   uiSrcIP;             // 探测源IP.
    UINT32   uiDestIP;            // 探测目的IP.
    UINT32   uiDscp;              // 探测流使用的DSCP.
    UINT32   uiSrcPortMin;        // 探测流源端口范围起始值, 需位于当前Agent已经保留的端口范围. 若为0时则使用agent保留的最小端口号.
    UINT32   uiSrcPortMax;        // 探测流源端口范围最大值, 需位于当前Agent已经保留的端口范围. 若为0时则使用agent保留的最大端口号.
    UINT32   uiSrcPortRange;      // 每个上报周期内覆盖的源端口个数. 若为0时则使用uiSrcPortMin to uiSrcPortMax计算
    UINT32   uiDestPort;
    // ServerAnt定义的拓扑信息, 来源于Server.
    ServerTopo_S stServerTopo;

    // 重载运算符, 方便进行key比较.
    bool operator == (const tagServerFlowKey & other) const
    {
        if (  (other.uiUrgentFlow == uiUrgentFlow)
              &&(other.eProtocol == eProtocol)
              &&(other.uiDestPort == uiDestPort)
              &&(other.uiSrcIP == uiSrcIP)
              &&(other.uiDestIP == uiDestIP)
              &&(other.uiDscp == uiDscp)
              &&(other.uiSrcPortMin == uiSrcPortMin)
              &&(other.uiSrcPortMax == uiSrcPortMax)
              &&(other.uiSrcPortRange == uiSrcPortRange)
              &&(other.stServerTopo == stServerTopo)
                )
        {
            return AGENT_TRUE;
        }
        else
        {
            return AGENT_FALSE;
        }
    }

    bool operator != (const tagServerFlowKey & other) const
    {
        if (  (other.uiUrgentFlow != uiUrgentFlow)
              ||(other.eProtocol != eProtocol)
              ||(other.uiDestPort != uiDestPort)
              ||(other.uiSrcIP != uiSrcIP)
              ||(other.uiDestIP != uiDestIP)
              ||(other.uiDscp != uiDscp)
              ||(other.uiSrcPortMin != uiSrcPortMin)
              ||(other.uiSrcPortMax != uiSrcPortMax)
              ||(other.uiSrcPortRange != uiSrcPortRange)
              ||(other.stServerTopo != stServerTopo)
                )
        {
            return AGENT_TRUE;
        }
        else
        {
            return AGENT_FALSE;
        }
    }
} ServerFlowKey_S;

// ServerFlowTable 支持添加, 不支持删除(会导致index错乱)
typedef struct tagServerFlowTableEntry
{
    // 流信息key, 来源于Server.
    ServerFlowKey_S   stServerFlowKey;

    // 当前ServerFlow对应AgentFlow中所有Entry的索引范围.
    UINT32   uiAgentFlowIndexMin;
    UINT32   uiAgentFlowIndexMax;

    // AgentFlow中当前enable的的流索引
    UINT32   uiAgentFlowWorkingIndexMin;
    UINT32   uiAgentFlowWorkingIndexMax;
} ServerFlowTableEntry_S;


// FlowManage类定义, 负责探测流表管理,及调度DetectWorker完成探测
class FlowManager_C : ThreadClass_C
{
private:
    // DetectWorker 资源管理
    // 初始化后不再修改, 不用互斥
    DetectWorker_C * WorkerList_UDP;                 // UDP Target Worker, 用于接收探测报文并发送应答报文.

    // Agent 使用的流表.
    // 操作工作流表需互斥, 操作配置流表无需互斥. 当前单线程场景实际不会触发互斥.
    vector <AgentFlowTableEntry_S> AgentFlowTable;   // 两个流表, 一个为工作流表, 另一个为配置流表, 互相独立, 配置完成后提交倒换.
    UINT32 uiAgentWorkingFlowTable;               // 当前工作流表, 0或1, 另外一个为配置流表.
    sal_mutex_t stAgentFlowTableLock;                   // 操作工作流表和修改uiWorkingFlowTable时需要互斥.

    // Agent流表处理
    INT32 AgentClearFlowTable();// 清空特定流表
    void AgentFlowTableAdd(ServerFlowTableEntry_S * pstServerFlowEntry);

    INT32 AgentRefreshFlowTable();                        // 刷新Agent流表, 由CommitServerCfgFlowTable()触发.
    INT32 AgentFlowTableEntryAdjust();                    // 根据range调整下一个report周期打开哪些流.
    void AgentFlowTableEntryClearResult(UINT32 uiAgentFlowIndex);                // 清空特定AgentFlow的探测结果


    // Server下发的流表
    // 操作工作流表需互斥, 操作配置流表无需互斥. 当前单线程场景实际不会触发互斥.
    vector <ServerFlowTableEntry_S> ServerFlowTable; // 工作流表
    UINT32 uiServerWorkingFlowTable;              // 当前工作流表, 0或1, 另外一个为配置流表.

    // Server流表处理
    INT32 ServerFlowTablePreAdd(
            ServerFlowKey_S * pstNewServerFlowKey,
            ServerFlowTableEntry_S * pstNewServerFlowEntry);    // 向ServerFlowTable中添加Entry前的预处理, 包括入参检查及参数初始化

    // 业务处理流程
    UINT32 uiNeedCheckResult;                     // 是否有未收集探测结果的流?
    UINT32 uiLastCheckTimeCounter;                // 最近一次启动探测流程的时间点
    INT32 DetectCheck(UINT32 counter);              // 检测此时是否该启动探测流程.
    INT32 DoDetect();                                     // 启动流探测.

    INT32 DetectResultCheck(UINT32 counter);        // 检测是此时否该检测探测结果. 探测启动时间+timeout时间.
    INT32 GetDetectResult();                              // 启动收集流探测结果.
    INT32 DetectResultProcess(UINT32 uiFlowTableIndex); // 每一次探测完成后的后续处理.
    INT32 FlowDropNotice(UINT32 uiFlowTableIndex);      // 持续丢包, 触发丢包快速上报,同时启动追踪报文.
    UINT32 uiLastReportTimeCounter;               // 最近一次启动上报Collector的时间点
    INT32 ReportCheck(UINT32 counter);              // 检测此时是否该启动上报Collector流程.
    INT32 QueryReportCheck(UINT32* flag, UINT32 uiCounter, UINT32 lastCounter, UINT32* failCounter);
    INT32 DoReport();                                     // 启动流上报.
    INT32 FlowComputeSD(
            INT64 * plSampleData,
            UINT32 uiSampleNumber,
            INT64 lSampleMeanValue,
            INT64 * plStandardDeviation);                    // 计算标准差
    INT32 FlowPrepareReport(UINT32 uiFlowTableIndex);   // 计算统计数据,准备上报
    INT32 FlowReportData(string * pstrReportData);      // 统一上报接口
    INT32 FlowDropReport(UINT32 uiFlowTableIndex, UINT32 bigPkgSize);      // 丢包上报接口
    INT32 FlowLatencyReport(UINT32 uiFlowTableIndex, UINT32 maxDelay, UINT32 bigPkgSize);   // 延时上报接口

    UINT32 uiLastQuerytTimeCounter;           // 最近一次启动查询Server的时间点
    UINT32 uiLastQueryConfigCounter;
    UINT32 uiLastReportIpCounter;
    INT32 DoQuery();                                  // 启动从Server刷新配置流程.

    /* Thread 实现代码 */
    INT32 ThreadHandler();                        // 任务主处理函数
    INT32 PreStopHandler();                       // StopThread触发, 通知ThreadHandler主动退出.
    INT32 PreStartHandler();                      // StartThread触发, 通知ThreadHandler即将被调用.
    INT32 DoQueryConfig();

public:
    FlowManager_C();                               // 构造函数, 填充默认值.
    FlowManager_C(ServerAntAgentCfg_C* pcNewAgentCfg);
    ~FlowManager_C();                              // 析构函数, 释放必要资源.

    // 全局AgentCfg信息
    ServerAntAgentCfg_C * pcAgentCfg;                   // agent_cfg

    INT32 Init(ServerAntAgentCfg_C * pcNewAgentCfg);   // 初始化函数

    INT32 ServerWorkingFlowTableAdd(ServerFlowKey_S *pstServerFlowKey);       // 向ServerWorkingFlowTable中添加Urgent Entry, 由Server下发消息触发

    void FlowManagerAction(UINT32 action);	    // 根据参数启停FlowManager
    void SetPkgFlag();
    void RefreshAgentTable();
    INT32 ServerClearFlowTable();   // 清空特定流表
};


#endif //PNTL_AGENT_FLOWMANAGER_H
