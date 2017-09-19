//
// Created by zy on 17-9-15.
//
#include <math.h>       // 计算标准差
#include <algorithm>    // 数组排序
#include <sys/time.h>   // 获取时间
#include <sstream>
#include <assert.h>
using namespace std;

#include "Log.h"
#include "AgentJsonAPI.h"
#include "GetLocalCfg.h"
#include "FlowManager.h"
#include "AgentCommon.h"
#include "MessagePlatformClient.h"

// 锁使用原则: 所有配置由ServerFlowTable刷新到AgentFlowTable.
// 如果要同时使用两个锁, 必须先获取SERVER_WORKING_FLOW_TABLE_LOCK()再获取AGENT_WORKING_FLOW_TABLE_LOCK,
// 释放时先释放AGENT_WORKING_FLOW_TABLE_UNLOCK(),再释放SERVER_WORKING_FLOW_TABLE_UNLOCK().

// Agent Flow Table
#define AGENT_WORKING_FLOW_TABLE_LOCK() \
        if (stAgentFlowTableLock) \
            sal_mutex_take(stAgentFlowTableLock, sal_mutex_FOREVER)

#define AGENT_WORKING_FLOW_TABLE_UNLOCK() \
        if (stAgentFlowTableLock) \
            sal_mutex_give(stAgentFlowTableLock)

#define AGENT_WORKING_FLOW_TABLE  (uiAgentWorkingFlowTable)


#define SERVER_WORKING_FLOW_TABLE  (uiServerWorkingFlowTable)


//Agent Flow Table Entry的uiFlowState bit定义
// 当前Entry是否生效.
#define FLOW_ENTRY_STATE_ENABLE     (1L << 0)
// 当前Entry是否处于丢包模式, 由丢包触发.
#define FLOW_ENTRY_STATE_DROPPING   (1L << 2)

#define FLOW_ENTRY_STATE_CHECK(state, flag)     ( (state) & (flag) )
#define FLOW_ENTRY_STATE_SET(state, flag)       ( (state) = ((state)|(flag)) )
#define FLOW_ENTRY_STATE_CLEAR(state, flag)     ( (state) = ((state)&(~(flag))) )

const UINT32 MAX_RETRY_TIMES = 100;

const UINT32 RETRY_INTERVAL = 5;

// 构造函数, 所有成员初始化默认值.
FlowManager_C::FlowManager_C()
{
    FLOW_MANAGER_INFO("Creat a new FlowManager");

    pcAgentCfg = NULL;

    // Worker 初始化
    WorkerList_UDP =NULL;

    // 流表处理
    uiAgentWorkingFlowTable = 0;
    AgentClearFlowTable();

    uiServerWorkingFlowTable = 0;
    ServerClearFlowTable();

    // 业务流程处理
    uiNeedCheckResult = 0;
    uiLastCheckTimeCounter = 0;
    uiLastReportTimeCounter = 0;
    uiLastQuerytTimeCounter = 0;
    uiLastQueryConfigCounter = 0;
    uiLastReportIpCounter = 0;
}

FlowManager_C::FlowManager_C(ServerAntAgentCfg_C* pcNewAgentCfg)
{
    FLOW_MANAGER_INFO("Creat a new FlowManager");

    pcAgentCfg = pcNewAgentCfg;

    // Worker 初始化
    WorkerList_UDP =NULL;

    // 流表处理
    uiAgentWorkingFlowTable = 0;
    AgentClearFlowTable();

    uiServerWorkingFlowTable = 0;
    ServerClearFlowTable();

    // 业务流程处理
    uiNeedCheckResult = 0;
    uiLastCheckTimeCounter = 0;
    uiLastReportTimeCounter = 0;
    uiLastQuerytTimeCounter = 0;
    uiLastQueryConfigCounter = 0;
    uiLastReportIpCounter = 0;
}

// 析构函数,释放资源
FlowManager_C::~FlowManager_C()
{
    FLOW_MANAGER_INFO("Destroy an old FlowManager");

    StopThread();
    // 清空流表
    AgentClearFlowTable();
    ServerClearFlowTable();

    stAgentFlowTableLock = NULL;
    if (WorkerList_UDP)
    {
        delete WorkerList_UDP;
    }
    WorkerList_UDP = NULL;
}

INT32 FlowManager_C::Init(ServerAntAgentCfg_C * pcNewAgentCfg)
{
    INT32 iRet = AGENT_OK;

    // UDP 协议初始化
    UINT32 uiSrcPortMin = 0;
    UINT32 uiSrcPortMax = 0;
    UINT32 uiDestPort = 0;
    WorkerCfg_S stNewWorker;

    if(NULL == pcNewAgentCfg)
    {
        FLOW_MANAGER_ERROR("Null Point.");
        return AGENT_E_PARA;
    }

    // 流表初始化
    pcAgentCfg = pcNewAgentCfg;
    pcAgentCfg->GetProtocolUDP(&uiSrcPortMin, &uiSrcPortMax, &uiDestPort);

    sal_memset(&stNewWorker, 0, sizeof(stNewWorker));
    stNewWorker.eProtocol  = AGENT_DETECT_PROTOCOL_UDP;
    stNewWorker.uiSrcPort   = uiSrcPortMin;
    stNewWorker.uiSrcIP     = SAL_INADDR_ANY;

    WorkerList_UDP = new DetectWorker_C;
    if( NULL == WorkerList_UDP )
    {
        return AGENT_E_MEMORY;
    }

    iRet = WorkerList_UDP->Init(stNewWorker, pcNewAgentCfg);
    if (iRet)
    {
        FLOW_MANAGER_ERROR("Init UDP target worker failed[%d], SIP[%s],SPort[%d]",
                           iRet, sal_inet_ntoa(stNewWorker.uiSrcIP), stNewWorker.uiSrcPort);
        return AGENT_E_PARA;
    }

    // 启动管理任务
    iRet = StartThread();
    if (iRet)
    {
        FLOW_MANAGER_ERROR("StartThread failed[%d]", iRet);
        return AGENT_E_PARA;
    }
    FLOW_MANAGER_INFO("init flow manager success");
    return iRet;
}

// Agent流表管理
// 清空特定流表
INT32 FlowManager_C::AgentClearFlowTable()
{

    // 清空每个流中的结果表.
    vector<AgentFlowTableEntry_S>::iterator pAgentFlowEntry;
    for(pAgentFlowEntry = AgentFlowTable.begin();
        pAgentFlowEntry != AgentFlowTable.end();
        pAgentFlowEntry ++)
    {
        pAgentFlowEntry->vFlowDetectResultPkt.clear();
    }

    // 清空整个流表.
    AgentFlowTable.clear();
    return AGENT_OK;
}


// 向AgentFlowTable中添加Entry
void FlowManager_C::AgentFlowTableAdd(ServerFlowTableEntry_S * pstServerFlowEntry)
{
    UINT32 uiSrcPort    = 0;
    UINT32 uiAgentIndexCounter    = 0;
    AgentFlowTableEntry_S stNewAgentEntry;

    // stNewAgentEntry中包含C++类, 不能直接使用sal_memset整体初始化. 未来考虑重构成对象.
    sal_memset(&(stNewAgentEntry.stFlowKey), 0, sizeof(stNewAgentEntry.stFlowKey));
    stNewAgentEntry.uiFlowState = 0;

    sal_memset(&(stNewAgentEntry.stFlowDetectResult), 0, sizeof(stNewAgentEntry.stFlowDetectResult));
    stNewAgentEntry.vFlowDetectResultPkt.clear();
    stNewAgentEntry.uiFlowDropCounter = 0;

    // 刷新key信息
    stNewAgentEntry.stFlowKey.uiUrgentFlow = pstServerFlowEntry->stServerFlowKey.uiUrgentFlow;
    stNewAgentEntry.stFlowKey.eProtocol = pstServerFlowEntry->stServerFlowKey.eProtocol;
    stNewAgentEntry.stFlowKey.uiSrcIP = pstServerFlowEntry->stServerFlowKey.uiSrcIP;
    stNewAgentEntry.stFlowKey.uiDestIP = pstServerFlowEntry->stServerFlowKey.uiDestIP;
    stNewAgentEntry.stFlowKey.uiDestPort = pstServerFlowEntry->stServerFlowKey.uiDestPort;
    stNewAgentEntry.stFlowKey.uiDscp = pstServerFlowEntry->stServerFlowKey.uiDscp;
    stNewAgentEntry.stFlowKey.stServerTopo = pstServerFlowEntry->stServerFlowKey.stServerTopo;
    if (this->pcAgentCfg->GetBigPkgRate())
    {
        stNewAgentEntry.stFlowKey.uiIsBigPkg = 1;
    }
    else
    {
        stNewAgentEntry.stFlowKey.uiIsBigPkg = 0;
    }


    // 刷新索引信息
    stNewAgentEntry.stFlowKey.uiAgentFlowTableIndex = AgentFlowTable.size();

    // 刷新Server Entry的Agent索引.
    pstServerFlowEntry->uiAgentFlowIndexMin = stNewAgentEntry.stFlowKey.uiAgentFlowTableIndex;
    pstServerFlowEntry->uiAgentFlowWorkingIndexMin = stNewAgentEntry.stFlowKey.uiAgentFlowTableIndex;
    pstServerFlowEntry->uiAgentFlowWorkingIndexMax = pstServerFlowEntry->uiAgentFlowWorkingIndexMin
                                                     + pstServerFlowEntry->stServerFlowKey.uiSrcPortRange - 1;

    for (uiSrcPort = pstServerFlowEntry->stServerFlowKey.uiSrcPortMin; uiSrcPort <= pstServerFlowEntry->stServerFlowKey.uiSrcPortMax; uiSrcPort++)
    {
        stNewAgentEntry.stFlowKey.uiSrcPort = uiSrcPort;

        // 默认打开uiSrcPortRange个流
        if (uiAgentIndexCounter < (pstServerFlowEntry->stServerFlowKey.uiSrcPortRange))
        {
            FLOW_ENTRY_STATE_SET(stNewAgentEntry.uiFlowState, FLOW_ENTRY_STATE_ENABLE);
        }
        else
        {
            FLOW_ENTRY_STATE_CLEAR(stNewAgentEntry.uiFlowState, FLOW_ENTRY_STATE_ENABLE);
        }

        AgentFlowTable.push_back(stNewAgentEntry);
        stNewAgentEntry.stFlowKey.uiAgentFlowTableIndex ++ ;
        uiAgentIndexCounter ++ ;
    }
    pstServerFlowEntry->uiAgentFlowIndexMax = stNewAgentEntry.stFlowKey.uiAgentFlowTableIndex - 1;
}

// 清空特定AgentFlow的探测结果
void FlowManager_C::AgentFlowTableEntryClearResult(UINT32 uiAgentFlowIndex)
{
    AgentFlowTable[uiAgentFlowIndex].uiFlowState = 0;
    AgentFlowTable[uiAgentFlowIndex].vFlowDetectResultPkt.clear();
    AgentFlowTable[uiAgentFlowIndex].uiFlowDropCounter = 0;
    sal_memset(&(AgentFlowTable[uiAgentFlowIndex].stFlowDetectResult), 0, sizeof(DetectResult_S));
}

// 根据range调整下一个上报周期打开哪些流.
INT32 FlowManager_C::AgentFlowTableEntryAdjust()
{
    INT32 iRet = AGENT_OK;
    UINT32 uiAgentFlowIndex = 0;
    UINT32 uiSrcPortRange = 0;

    // 遍历工作ServerFlowTable
    vector<ServerFlowTableEntry_S>::iterator pServerEntry;
    for(pServerEntry = ServerFlowTable.begin();
        pServerEntry != ServerFlowTable.end();
        pServerEntry++)
    {
        // 关闭本ServerFlow对应的所有的AgentFlow, 并且清空对应流统计和状态
        for ( uiAgentFlowIndex = pServerEntry->uiAgentFlowIndexMin;
              uiAgentFlowIndex <= pServerEntry->uiAgentFlowIndexMax;
              uiAgentFlowIndex ++)
        {
            AgentFlowTableEntryClearResult(uiAgentFlowIndex);
        }
        // 根据range计算下一轮探测的AgentFlow
        if (0 != pcAgentCfg->GetPortCount())
        {
            uiSrcPortRange = pcAgentCfg->GetPortCount();
        }
        else
        {
            uiSrcPortRange = pServerEntry->stServerFlowKey.uiSrcPortRange;
        }

        if ( pServerEntry->uiAgentFlowWorkingIndexMax + uiSrcPortRange <= pServerEntry->uiAgentFlowIndexMax)
        {
            pServerEntry->uiAgentFlowWorkingIndexMin = pServerEntry->uiAgentFlowWorkingIndexMax + 1;
            pServerEntry->uiAgentFlowWorkingIndexMax = pServerEntry->uiAgentFlowWorkingIndexMax + uiSrcPortRange;

            // 打开下一轮需要探测的AgentFlow
            for ( uiAgentFlowIndex = pServerEntry->uiAgentFlowWorkingIndexMin;
                  uiAgentFlowIndex <= pServerEntry->uiAgentFlowWorkingIndexMax;
                  uiAgentFlowIndex ++)
            {
                FLOW_ENTRY_STATE_SET(AgentFlowTable[uiAgentFlowIndex].uiFlowState, FLOW_ENTRY_STATE_ENABLE);
            }
        }
        else
        {
            if (pServerEntry->uiAgentFlowWorkingIndexMax + 1 <= pServerEntry->uiAgentFlowIndexMax)
            {
                pServerEntry->uiAgentFlowWorkingIndexMin = pServerEntry->uiAgentFlowWorkingIndexMax + 1;
                pServerEntry->uiAgentFlowWorkingIndexMax = pServerEntry->uiAgentFlowIndexMin +
                                                           pServerEntry->uiAgentFlowWorkingIndexMax + uiSrcPortRange - pServerEntry->uiAgentFlowIndexMax - 1;

                // 打开下一轮需要探测的AgentFlow
                for ( uiAgentFlowIndex = pServerEntry->uiAgentFlowWorkingIndexMin;
                      uiAgentFlowIndex <= pServerEntry->uiAgentFlowIndexMax;
                      uiAgentFlowIndex ++)
                {
                    FLOW_ENTRY_STATE_SET(AgentFlowTable[uiAgentFlowIndex].uiFlowState, FLOW_ENTRY_STATE_ENABLE);
                }
                for ( uiAgentFlowIndex = pServerEntry->uiAgentFlowIndexMin;
                      uiAgentFlowIndex <= pServerEntry->uiAgentFlowWorkingIndexMax;
                      uiAgentFlowIndex ++)
                {
                    FLOW_ENTRY_STATE_SET(AgentFlowTable[uiAgentFlowIndex].uiFlowState,  FLOW_ENTRY_STATE_ENABLE);
                }

            }
            else
            {
                pServerEntry->uiAgentFlowWorkingIndexMin = pServerEntry->uiAgentFlowIndexMin;
                pServerEntry->uiAgentFlowWorkingIndexMax = pServerEntry->uiAgentFlowIndexMin + uiSrcPortRange - 1;
                // 打开下一轮需要探测的AgentFlow
                for ( uiAgentFlowIndex = pServerEntry->uiAgentFlowWorkingIndexMin;
                      uiAgentFlowIndex <= pServerEntry->uiAgentFlowWorkingIndexMax;
                      uiAgentFlowIndex ++)
                {
                    FLOW_ENTRY_STATE_SET(AgentFlowTable[uiAgentFlowIndex].uiFlowState, FLOW_ENTRY_STATE_ENABLE);
                }
            }
        }
    }
    return AGENT_OK;
}

// Server流表管理
// 清空特定流表
INT32 FlowManager_C::ServerClearFlowTable()
{
    // 清空整个流表.
    ServerFlowTable.clear();
    return AGENT_OK;
}


// 向ServerFlowTable中添加Entry前的预处理, 包括入参检查及参数初始化
INT32 FlowManager_C::ServerFlowTablePreAdd(ServerFlowKey_S * pstNewServerFlowKey, ServerFlowTableEntry_S * pstNewServerFlowEntry)
{
    INT32 iRet = AGENT_OK;
    UINT32 uiSrcPortMin = 0;
    UINT32 uiSrcPortMax = 0;
    UINT32 uiAgentSrcPortRange = 0;
    UINT32 uiDestPort = 0;

    sal_memset(pstNewServerFlowEntry, 0, sizeof(ServerFlowTableEntry_S));

    if (AGENT_DETECT_PROTOCOL_UDP == pstNewServerFlowKey->eProtocol)
    {
        // 获取当前Agent全局源端口范围.
        pcAgentCfg->GetProtocolUDP(&uiSrcPortMin, &uiSrcPortMax, &uiDestPort);

        // 填充源端口号默认值

        if (0 == pstNewServerFlowKey->uiSrcPortMin)
        {
            FLOW_MANAGER_INFO("SrcPortMin is 0, Using Default SrcPortMin[%u].",
                              uiSrcPortMin);
            pstNewServerFlowKey->uiSrcPortMin = uiSrcPortMin;
        }

        if (0 == pstNewServerFlowKey->uiSrcPortMax)
        {
            FLOW_MANAGER_INFO("SrcPortMax is 0, Using Default SrcPortMin[%u].",
                              uiSrcPortMax);
            pstNewServerFlowKey->uiSrcPortMax = uiSrcPortMax;
        }

        pstNewServerFlowKey->uiDestPort = uiDestPort;

        uiAgentSrcPortRange = pstNewServerFlowKey->uiSrcPortMax - pstNewServerFlowKey->uiSrcPortMin + 1;

        if (0 == pstNewServerFlowKey->uiSrcPortRange)
        {
            FLOW_MANAGER_INFO("SrcPortRange is 0, Using Default SrcPortMin[%u]: [%u]-[%u].",
                              uiAgentSrcPortRange, pstNewServerFlowKey->uiSrcPortMin, pstNewServerFlowKey->uiSrcPortMax);
            pstNewServerFlowKey->uiSrcPortRange = uiAgentSrcPortRange;
        }

        // 入参检查
        if (pstNewServerFlowKey->uiSrcPortMin > pstNewServerFlowKey->uiSrcPortMax)

        {
            FLOW_MANAGER_ERROR("SrcPortMin[%u] is bigger than SrcPortMax[%u]",
                               pstNewServerFlowKey->uiSrcPortMin, pstNewServerFlowKey->uiSrcPortMax);
            return AGENT_E_PARA;
        }

        if (  (uiSrcPortMin > pstNewServerFlowKey->uiSrcPortMin)
              ||(uiSrcPortMax < pstNewServerFlowKey->uiSrcPortMax))
        {
            FLOW_MANAGER_ERROR("SrcPortMin[%u] - SrcPortMax[%u] from server is too larger than this agent: SrcPortMin[%u] - SrcPortMax[%u]",
                               pstNewServerFlowKey->uiSrcPortMin, pstNewServerFlowKey->uiSrcPortMax, uiSrcPortMin, uiSrcPortMax);
            return AGENT_E_PARA;
        }

        // 检查Range范围
        if ( uiAgentSrcPortRange < pstNewServerFlowKey->uiSrcPortRange )
        {
            FLOW_MANAGER_ERROR("SrcPortRange[%u] from server is too larger than Flow range [%u]: SrcPortMin[%u] - SrcPortMax[%u]",
                               pstNewServerFlowKey->uiSrcPortRange, uiAgentSrcPortRange, pstNewServerFlowKey->uiSrcPortMin, pstNewServerFlowKey->uiSrcPortMax);
            return AGENT_E_PARA;
        }

        // 检查dscp是否合法
        if (pstNewServerFlowKey->uiDscp > AGENT_MAX_DSCP_VALUE)
        {
            FLOW_MANAGER_ERROR("Dscp[%d] is bigger than the max value[%d]", pstNewServerFlowKey->uiDscp, AGENT_MAX_DSCP_VALUE);
            return AGENT_E_PARA;
        }
        pstNewServerFlowEntry->stServerFlowKey = * pstNewServerFlowKey;
    }
    else
    {
        FLOW_MANAGER_ERROR("Unsupported Protocol[%d]", pstNewServerFlowKey->eProtocol);
        iRet = AGENT_E_PARA;
    }
    return iRet;
}


// 向ServerWorkingFlowTable中添加Entry, 由Server下发消息触发. 一般用于添加Urgent Entry
INT32 FlowManager_C::ServerWorkingFlowTableAdd(ServerFlowKey_S *pstNewServerFlowKey)
{
    INT32 iRet = AGENT_OK;
    ServerFlowTableEntry_S stNewServerFlowEntry;

    iRet = ServerFlowTablePreAdd(pstNewServerFlowKey, &stNewServerFlowEntry);
    if (iRet)
    {
        FLOW_MANAGER_ERROR("Server Flow Table Pre Add failed[%d]", iRet);
        return iRet;
    }

    // 检查工作表中是否有重复表项, Urgent可以重复
    vector<ServerFlowTableEntry_S>::iterator pServerFlowEntry;
    for(pServerFlowEntry = ServerFlowTable.begin();
        pServerFlowEntry != ServerFlowTable.end();
        pServerFlowEntry++)
    {
        if (stNewServerFlowEntry.stServerFlowKey == pServerFlowEntry->stServerFlowKey)
        {
            FLOW_MANAGER_WARNING("This flow already exist");
            return AGENT_OK;
        }
    }
    // ServerTable中添加一份记录, 以免query周期到达刷新agent表时将尚未完成探测的Urgent流清除.
    ServerFlowTable.push_back(stNewServerFlowEntry);

    return iRet;
}

// 检测此时是否该启动探测流程.
INT32 FlowManager_C::DetectCheck(UINT32 uiCounter)
{
    UINT32 uiTimeCost = 0 ;

    // uiCounter 可能溢出
    uiTimeCost = (uiCounter >= uiLastCheckTimeCounter)
                 ? uiCounter - uiLastCheckTimeCounter
                 : uiCounter + ((UINT32)(-1) - uiLastCheckTimeCounter + 1);

    if (SHOULD_DETECT_REPORT && uiTimeCost >= pcAgentCfg->GetDetectPeriod())
    {
        uiLastCheckTimeCounter = uiCounter;
        uiNeedCheckResult = AGENT_ENABLE;
        return AGENT_ENABLE;
    }
    else
    {
        return AGENT_DISABLE;
    }
}

// 启动流探测.
INT32 FlowManager_C::DoDetect()
{
    INT32 iRet = AGENT_OK;
    FlowKey_S stFlowKey;

    // 当前只处理udp协议, 获取udp worker list大小
    vector<AgentFlowTableEntry_S>::iterator pAgentFlowEntry;
    for(pAgentFlowEntry = AgentFlowTable.begin();
        pAgentFlowEntry != AgentFlowTable.end();
        pAgentFlowEntry++)
    {
        // 当前只处理udp协议
        if (AGENT_DETECT_PROTOCOL_UDP == pAgentFlowEntry->stFlowKey.eProtocol)
        {
            // 只处理enable的Entry
            if (FLOW_ENTRY_STATE_CHECK(pAgentFlowEntry->uiFlowState, FLOW_ENTRY_STATE_ENABLE))
            {
                // 检查越界,避免踩内存.
                if (NULL !=  WorkerList_UDP)
                {
                    stFlowKey = pAgentFlowEntry->stFlowKey;
                    iRet = WorkerList_UDP->PushSession(stFlowKey);

                    if (iRet && AGENT_E_SOCKET != iRet)
                    {
                        FLOW_MANAGER_ERROR("Push Session to Worker Failed[%d], SrcPort[%u]", iRet, pAgentFlowEntry->stFlowKey.uiSrcPort);
                        continue;
                    }
                }
                else
                {
                    FLOW_MANAGER_ERROR("UDP  is over udp worker list, SrcPort[%u].", pAgentFlowEntry->stFlowKey.uiSrcPort);
                    continue;
                }
            }
        }
    }

    if (AGENT_E_SOCKET == iRet)
    {
        iRet = AGENT_OK;
    }

    return iRet;
}

// 检测是此时否该检测探测结果. 探测启动时间+timeout时间.
INT32 FlowManager_C::DetectResultCheck(UINT32 uiCounter)
{
    UINT32 uiTimeCost = 0 ;

    // uiCounter 可能溢出
    uiTimeCost = (uiCounter >= uiLastCheckTimeCounter)
                 ? uiCounter - uiLastCheckTimeCounter
                 : uiCounter + ((UINT32)(-1) - uiLastCheckTimeCounter + 1);

    if ( AGENT_ENABLE == uiNeedCheckResult
         &&(uiTimeCost >= pcAgentCfg->GetDetectTimeout()))
    {
        uiNeedCheckResult = AGENT_FALSE;
        return AGENT_ENABLE;
    }
    else
    {
        return AGENT_DISABLE;
    }
}

// 计算标准差
INT32 FlowManager_C::FlowComputeSD(INT64 * plSampleData, UINT32 uiSampleNumber, INT64 lSampleMeanValue,
                                   INT64 * plStandardDeviation)
{
    UINT32 uiSampleIndex = 0;
    INT64 lVariance = 0;  //方差

    assert(0 != uiSampleNumber);

    *plStandardDeviation = 0;

    for (uiSampleIndex = 0; uiSampleIndex < uiSampleNumber; uiSampleIndex ++)
    {
        lVariance += pow((plSampleData[uiSampleIndex] - lSampleMeanValue), 2);
    }
    lVariance = lVariance / uiSampleNumber; // 方差
    *plStandardDeviation = sqrt(lVariance); // 标准差

    return AGENT_OK;
}

// 计算统计数据,准备上报
INT32 FlowManager_C::FlowPrepareReport(UINT32 uiFlowTableIndex)
{
    INT32 iRet = AGENT_OK;

    // 探测样本总数.
    UINT32 uiResultNumber = 0;

    // 有效样本总数.
    UINT32 uiSampleNumber = 0;

    INT64 lDataTemp = 0;         // 缓存数据.
    INT64 * plT3Temp = NULL;     // 时延 us, Target平均处理时间(stT3 - stT2)
    INT64 lT3Sum = 0;
    INT64 * plT4Temp = NULL;     // 时延 us, Sender报文平均往返时间(stT4 - stT1), RTT
    INT64 lT4Sum = 0;
    INT64 lStandardDeviation = 0;// 时延标准差


    // 探测样本总数.
    uiResultNumber = AgentFlowTable[uiFlowTableIndex].vFlowDetectResultPkt.size();

    // 填充数据生成时间
    {
        struct timeval tm;
        sal_memset(&tm, 0, sizeof(tm));
        gettimeofday(&tm,NULL); // 获取当前时间
        // 以ms为单位进行上报.
        AgentFlowTable[uiFlowTableIndex].stFlowDetectResult.lT5 = (INT64)tm.tv_sec * SECOND_MSEC
                                                                  + (INT64)tm.tv_usec / MILLISECOND_USEC;
    }

    // 没有探测样本
    if (0 == uiResultNumber)
    {
        return AGENT_OK;
    }

    plT3Temp = new INT64[uiResultNumber];
    if (NULL == plT3Temp)
    {

        FLOW_MANAGER_ERROR("No enough memory.[%u]", uiResultNumber);
        return AGENT_E_MEMORY;
    }

    plT4Temp = new INT64[uiResultNumber];
    if (NULL == plT4Temp)
    {
        delete [] plT3Temp;
        plT3Temp = NULL;

        FLOW_MANAGER_ERROR("No enough memory.[%u]", uiResultNumber);
        return AGENT_E_MEMORY;
    }

    sal_memset(plT3Temp, 0, sizeof(INT64)*uiResultNumber);
    sal_memset(plT4Temp, 0, sizeof(INT64)*uiResultNumber);

    // 计算时延, 剔除没有收到应答的数据
    vector<DetectResultPkt_S>::iterator pDetectResultPkt;
    for(pDetectResultPkt = AgentFlowTable[uiFlowTableIndex].vFlowDetectResultPkt.begin();
        pDetectResultPkt != AgentFlowTable[uiFlowTableIndex].vFlowDetectResultPkt.end();
        pDetectResultPkt ++)
    {
        if ( SESSION_STATE_WAITING_CHECK == pDetectResultPkt->uiSessionState )
        {
            // 小于0的时延不存在, 可能os在调整系统时间, 忽略该异常数据.
            // 单跳时延在us级, 使用us进行计算.
            lDataTemp = pDetectResultPkt->stT3 - pDetectResultPkt->stT2;
            if (0 <= lDataTemp)
            {
                plT3Temp[uiSampleNumber] = lDataTemp; //us
                lT3Sum += lDataTemp;
            }

            lDataTemp = pDetectResultPkt->stT4 - pDetectResultPkt->stT1;
            if (0 <= lDataTemp)
            {
                plT4Temp[uiSampleNumber] = lDataTemp; //us
                lT4Sum += lDataTemp;
            }
            uiSampleNumber ++;
        }
    }
    // 没有收到一个有效应答报文, 没有有效时延数据.
    if (0 == uiSampleNumber)
    {
        delete [] plT3Temp;
        plT3Temp = NULL;
        delete [] plT4Temp;
        plT4Temp = NULL;

        // 设置时间信息为-1.
        AgentFlowTable[uiFlowTableIndex].stFlowDetectResult.lT2 = -1;
        AgentFlowTable[uiFlowTableIndex].stFlowDetectResult.lT3 = -1;
        AgentFlowTable[uiFlowTableIndex].stFlowDetectResult.lT4 = -1;
        AgentFlowTable[uiFlowTableIndex].stFlowDetectResult.lLatencyMin = -1;
        AgentFlowTable[uiFlowTableIndex].stFlowDetectResult.lLatencyMax = -1;
        AgentFlowTable[uiFlowTableIndex].stFlowDetectResult.lLatency50Percentile = -1;
        AgentFlowTable[uiFlowTableIndex].stFlowDetectResult.lLatency99Percentile = -1;

        return AGENT_OK;
    }
    // 对plT3Temp(Target) 计算平均值
    AgentFlowTable[uiFlowTableIndex].stFlowDetectResult.lT3 = lT3Sum / uiSampleNumber;


    // 对plT4Temp(rtt) 计算平均值.
    lDataTemp = lT4Sum / uiSampleNumber;
    AgentFlowTable[uiFlowTableIndex].stFlowDetectResult.lT4 = lDataTemp;


    // 对plT4Temp(rtt) 计算标准差
    iRet = FlowComputeSD(plT4Temp, uiSampleNumber, lDataTemp, &lStandardDeviation);
    if (iRet)
    {
        delete [] plT3Temp;
        plT3Temp = NULL;
        delete [] plT4Temp;
        plT4Temp = NULL;

        FLOW_MANAGER_ERROR("Flow Sort Result failed[%d]", iRet);
        return iRet;
    }
    AgentFlowTable[uiFlowTableIndex].stFlowDetectResult.lLatencyStandardDeviation = lStandardDeviation;

    // 对plT4Temp(rtt) 进行从小到大排序
    sort(plT4Temp, plT4Temp + uiSampleNumber);

    // 获取plT4Temp(rtt)最小值
    lDataTemp = 0;
    AgentFlowTable[uiFlowTableIndex].stFlowDetectResult.lLatencyMin = plT4Temp[lDataTemp];

    // 获取plT4Temp(rtt)最大值
    // uiSampleNumber 非 0
    lDataTemp = uiSampleNumber - 1;
    AgentFlowTable[uiFlowTableIndex].stFlowDetectResult.lLatencyMax = plT4Temp[lDataTemp];

    // 获取plT4Temp(rtt)中位数
    if( 2 <= uiSampleNumber)
    {
        lDataTemp = uiSampleNumber / 2 - 1;
    }
    else
    {
        lDataTemp = 0;
    }
    AgentFlowTable[uiFlowTableIndex].stFlowDetectResult.lLatency50Percentile = plT4Temp[lDataTemp];

    // 获取plT4Temp(rtt)99%位数
    if( 2 <= uiSampleNumber)
    {
        lDataTemp = uiSampleNumber * 99 / 100 - 1;
    }
    else
    {
        lDataTemp = 0;
        AgentFlowTable[uiFlowTableIndex].stFlowDetectResult.lLatency99Percentile = plT4Temp[lDataTemp];
    }


    delete [] plT3Temp;
    plT3Temp = NULL;
    delete [] plT4Temp;
    plT4Temp = NULL;

    return AGENT_OK;
}



// 丢包上报接口
INT32 FlowManager_C::FlowDropReport(UINT32 uiFlowTableIndex, UINT32 bigPkgSize)
{
    INT32 iRet = AGENT_OK;
    AgentFlowTableEntry_S * pstAgentFlowEntry = NULL;
    stringstream  ssReportData; // 用于生成json格式上报数据

    iRet = FlowPrepareReport(uiFlowTableIndex);
    if (iRet)
    {
        FLOW_MANAGER_ERROR("Flow Prepare Report failed[%d]", iRet);
        return iRet;
    }

    ssReportData.clear();
    ssReportData.str("");

    pstAgentFlowEntry = &(AgentFlowTable[uiFlowTableIndex]);
    pstAgentFlowEntry->stFlowDetectResult.lDropNotesCounter ++;

    iRet = CreateDropReportData(pstAgentFlowEntry, &ssReportData, bigPkgSize);
    if (iRet)
    {
        FLOW_MANAGER_ERROR("Create Drop Report Data failed[%d]", iRet);
        return iRet;
    }


    iRet = ReportDataToServer(pcAgentCfg, &ssReportData, KAFKA_TOPIC_URL + this->pcAgentCfg->GetTopic());
    if (iRet)
    {
        FLOW_MANAGER_ERROR("Flow Report Data failed[%d]", iRet);
        return iRet;
    }
    return AGENT_OK;
}

// 延时上报接口
INT32 FlowManager_C::FlowLatencyReport(UINT32 uiFlowTableIndex, UINT32 maxDelay, UINT32 bigPkgSize)
{
    INT32 iRet = AGENT_OK;
    AgentFlowTableEntry_S * pstAgentFlowEntry = NULL;
    stringstream  ssReportData; // 用于生成json格式上报数据
    string strReportData;       // 用于缓存用于上报的字符串信息

    iRet = FlowPrepareReport(uiFlowTableIndex);
    if (iRet)
    {
        FLOW_MANAGER_ERROR("Flow Prepare Report failed[%d]", iRet);
        return iRet;
    }

    ssReportData.clear();
    ssReportData.str("");

    pstAgentFlowEntry = &(AgentFlowTable[uiFlowTableIndex]);
    iRet = CreateLatencyReportData(pstAgentFlowEntry, &ssReportData, maxDelay, bigPkgSize);

    if (AGENT_FILTER_DELAY == iRet)
    {
        return AGENT_FILTER_DELAY;
    }
    else if (iRet)
    {
        FLOW_MANAGER_ERROR("Creat Latency Report Data[%d]", iRet);
        return iRet;
    }
    strReportData = ssReportData.str();
    iRet = ReportDataToServer(pcAgentCfg, &ssReportData, KAFKA_TOPIC_URL + this->pcAgentCfg->GetTopic());
    if (iRet)
    {
        FLOW_MANAGER_ERROR("Flow Report Data failed[%d]", iRet);
        return iRet;
    }
    return AGENT_OK;
}

// 持续丢包, 触发丢包快速上报,同时启动追踪报文.
INT32 FlowManager_C::FlowDropNotice(UINT32 uiFlowTableIndex)
{
    INT32 iRet = AGENT_OK;

    FLOW_ENTRY_STATE_SET(AgentFlowTable[uiFlowTableIndex].uiFlowState, FLOW_ENTRY_STATE_DROPPING);

    iRet = FlowDropReport(uiFlowTableIndex, pcAgentCfg->GetBigPkgSize());
    if (iRet)
    {
        FLOW_MANAGER_ERROR("Flow Drop Report failed[%d]", iRet);
    }

    return iRet;
}

// 每一次探测完成后的后续处理.
INT32 FlowManager_C::DetectResultProcess(UINT32 uiFlowTableIndex)
{
    INT32 iRet = AGENT_OK;
    DetectResultPkt_S stDetectResultPkt;
    UINT32 uiUrgentFlow = 0;


    // 获取刚刚压入的最新探测结果.
    stDetectResultPkt = AgentFlowTable[uiFlowTableIndex].vFlowDetectResultPkt.back();

    // 如果是第一个探测报文, 刷新探测时间.
    // 若第一个报文发送失败,则lT1=0.
    // 若第一个报文发送成功,但是丢包. 则lT1为发送时间, lT2=0.
    if (1 == AgentFlowTable[uiFlowTableIndex].vFlowDetectResultPkt.size())
    {

        // 时间戳以ms为单位进行上报.
        AgentFlowTable[uiFlowTableIndex].stFlowDetectResult.lT1 = (INT64)stDetectResultPkt.stT1.uiSec * SECOND_MSEC
                                                                  + (INT64)stDetectResultPkt.stT1.uiUsec / MILLISECOND_USEC;
        AgentFlowTable[uiFlowTableIndex].stFlowDetectResult.lT2 = (INT64)stDetectResultPkt.stT2.uiSec * SECOND_MSEC
                                                                  + (INT64)stDetectResultPkt.stT2.uiUsec / MILLISECOND_USEC;
    }

    // 刷新流表统计信息
    // 发送成功
    if ( SESSION_STATE_SEND_FAIELD != stDetectResultPkt.uiSessionState)
    {
        AgentFlowTable[uiFlowTableIndex].stFlowDetectResult.lPktSentCounter ++;
    }

    // 丢包
    if ( SESSION_STATE_TIMEOUT == stDetectResultPkt.uiSessionState )
    {
        AgentFlowTable[uiFlowTableIndex].stFlowDetectResult.lPktDropCounter ++;
        AgentFlowTable[uiFlowTableIndex].uiFlowDropCounter ++;

    }
    else if ( SESSION_STATE_WAITING_CHECK == stDetectResultPkt.uiSessionState ) //未丢包
    {
        // 取消丢包状态.
        AgentFlowTable[uiFlowTableIndex].uiFlowDropCounter = 0;
        FLOW_ENTRY_STATE_CLEAR(AgentFlowTable[uiFlowTableIndex].uiFlowState, FLOW_ENTRY_STATE_DROPPING);
    }

    // 普通流持续丢包, 触发丢包快速上报,同时启动追踪报文.
    if ( !(FLOW_ENTRY_STATE_CHECK(AgentFlowTable[uiFlowTableIndex].uiFlowState, FLOW_ENTRY_STATE_DROPPING))
         && (pcAgentCfg->GetDetectDropThresh() <= AgentFlowTable[uiFlowTableIndex].uiFlowDropCounter))
    {
        // 进入丢包状态
        iRet = FlowDropNotice(uiFlowTableIndex);
        if (iRet)
        {
            FLOW_MANAGER_ERROR("FlowDropNotice failed[%d], index[%d]", iRet, uiFlowTableIndex);
        }
    }

    return AGENT_OK;
}

// 启动收集流探测结果.
INT32 FlowManager_C::GetDetectResult()
{
    INT32 iRet = AGENT_OK;
    UINT32 uiAgentFlowTableSize = 0;
    UINT32 uiAgentFlowTableIndex = 0;
    DetectWorkerSession_S stWorkerSession;
    DetectResultPkt_S   stDetectResultPkt;
    // 当前只处理udp协议, 遍历 udp worker list

    // 处理该worker中所有待收集的会话.
    if (NULL  == WorkerList_UDP)
    {
        return AGENT_E_NOT_FOUND;
    }

    // 探测结果会回写流表.

    uiAgentFlowTableSize = AgentFlowTable.size();

    do
    {
        sal_memset(&stWorkerSession, 0, sizeof(stWorkerSession));

        iRet = WorkerList_UDP->PopSession(&stWorkerSession);
        if( iRet && (AGENT_E_NOT_FOUND != iRet))
        {
            FLOW_MANAGER_ERROR("Worker PopSession Failed[%d]", iRet);
            continue;
        }
        else if (AGENT_E_NOT_FOUND == iRet)
        {
            break;
        }
        uiAgentFlowTableIndex = stWorkerSession.stFlowKey.uiAgentFlowTableIndex;
        // 检查FlowTableIndex是否有效
        if (uiAgentFlowTableIndex < uiAgentFlowTableSize)
        {
            sal_memset(&stDetectResultPkt, 0, sizeof(stDetectResultPkt));
            stDetectResultPkt.uiSessionState = stWorkerSession.uiSessionState;
            stDetectResultPkt.uiSequenceNumber = stWorkerSession.uiSequenceNumber;
            stDetectResultPkt.stT1 = stWorkerSession.stT1;
            stDetectResultPkt.stT2 = stWorkerSession.stT2;
            stDetectResultPkt.stT3 = stWorkerSession.stT3;
            stDetectResultPkt.stT4 = stWorkerSession.stT4;

            if (0 == FLOW_ENTRY_STATE_CHECK(AgentFlowTable[uiAgentFlowTableIndex].uiFlowState, FLOW_ENTRY_STATE_ENABLE))
            {
                continue;
            }

            // 向流表写入探测结果
            AgentFlowTable[uiAgentFlowTableIndex].vFlowDetectResultPkt.push_back(stDetectResultPkt);

            // 针对刚加入的探测结果进行处理, 进行丢包,Urgent等事件处理.
            iRet = DetectResultProcess(uiAgentFlowTableIndex);
            if (iRet)
            {
                FLOW_MANAGER_WARNING("DetectResultProcess failed [%d]", iRet);
                continue;
            }
        }
        else
        {
            FLOW_MANAGER_INFO("FlowTableIndex[%u] is over size[%d]. Maybe DoQuery just clear the agent flow table",
                              uiAgentFlowTableIndex, uiAgentFlowTableSize);
            continue;
        }

    }
    while( AGENT_E_NOT_FOUND != iRet );

    return AGENT_OK;
}

// 检测此时是否该启动上报Collector流程.
INT32 FlowManager_C::ReportCheck(UINT32 uiCounter)
{
    UINT32 uiTimeCost = 0 ;

    // uiCounter 可能溢出
    uiTimeCost = (uiCounter >= uiLastReportTimeCounter)
                 ? uiCounter - uiLastReportTimeCounter
                 : uiCounter + ((UINT32)(-1) - uiLastReportTimeCounter + 1);

    if (SHOULD_DETECT_REPORT && uiTimeCost >= pcAgentCfg->GetReportPeriod())
    {
        uiLastReportTimeCounter = uiCounter;
        return AGENT_ENABLE;
    }
    else
    {
        return AGENT_DISABLE;
    }
}

// 启动流探测结果上报.
INT32 FlowManager_C::DoReport()
{
    INT32 iRet = AGENT_OK;
    UINT32 uiFlowTableIndex = 0;

    for(uiFlowTableIndex = 0; uiFlowTableIndex < AgentFlowTable.size(); uiFlowTableIndex++)
    {
        if ((FLOW_ENTRY_STATE_CHECK(AgentFlowTable[uiFlowTableIndex].uiFlowState, FLOW_ENTRY_STATE_ENABLE)))
        {
            iRet = FlowLatencyReport(uiFlowTableIndex, pcAgentCfg->GetMaxDelay(), pcAgentCfg->GetBigPkgSize());
            if (AGENT_FILTER_DELAY == iRet)
            {
                continue ;
            }
            else if (iRet)
            {
                FLOW_MANAGER_ERROR("Flow Latency Report failed[%d], index[%u]", iRet, uiFlowTableIndex);
            }
        }

    }
    // 根据range调整下一个上报周期使能AgentFlowTable中的哪些流.
    iRet = AgentFlowTableEntryAdjust();
    if (iRet)
    {
        FLOW_MANAGER_ERROR("Flow Working Entry Adjust failed[%d]", iRet);
    }
    return iRet;
}

// 检测此时是否该启动查询pinglist流程.
INT32 FlowManager_C::QueryReportCheck(UINT32* flag, UINT32 uiCounter, UINT32 lastCounter, UINT32* failCounter)
{
    if (*flag && 0 == *(failCounter))
    {
        return AGENT_ENABLE;
    }
    if (*flag && *(failCounter) < MAX_RETRY_TIMES)
    {
        return uiCounter == lastCounter + *(failCounter) * RETRY_INTERVAL;
    }
    else
    {
        *flag = 0;
        *(failCounter) = 0;
        return AGENT_DISABLE;
    }
}

void FlowManager_C::RefreshAgentTable()
{
    // 清空Agent配置表
    AgentClearFlowTable();

    // 检查工作表中是否有重复表项, Urgent可以重复
    vector<ServerFlowTableEntry_S>::iterator pServerFlowEntry;
    for(pServerFlowEntry = ServerFlowTable.begin();
        pServerFlowEntry != ServerFlowTable.end();
        pServerFlowEntry++)
    {
        AgentFlowTableAdd(&(*pServerFlowEntry));
    }
}

// Thread回调函数.
// PreStopHandler()执行后, ThreadHandler()需要在GetCurrentInterval() us内主动退出.
INT32 FlowManager_C::ThreadHandler()
{
    INT32 iRet = AGENT_OK;
    UINT32 counter = 0;
    uiLastQueryConfigCounter = counter;
    uiLastReportIpCounter = counter;
    uiLastQuerytTimeCounter = counter;
    uiLastCheckTimeCounter = counter;
    uiLastReportTimeCounter = counter;
    UINT32 uiQueryPingListFailCounter = 0;
    UINT32 uiQueryConfFailCounter = 0;
    UINT32 uiReportIpFailCounter = 0;

    while (GetCurrentInterval())
    {
        // 当前周期是否该启动探测流程.
        if (DetectCheck(counter))
        {
            // 启动探测流程.
            iRet = DoDetect();
            if (iRet)
            {
                FLOW_MANAGER_WARNING("Do UDP Detect failed[%d]", iRet);
            }
        }

        // 当前周期是否该收集探测结果
        if (DetectResultCheck(counter))
        {
            // 启动收集探测结果流程
            iRet = GetDetectResult();
            if (iRet)
            {
                FLOW_MANAGER_WARNING("Get UDP Detect Result failed[%d]", iRet);
            }
        }

        // 当前周期是否该启动上报Collector流程
        if (ReportCheck(counter))
        {
            // 启动上报Collector流程
            iRet = DoReport();
            if (iRet)
            {
                FLOW_MANAGER_WARNING("Do UDP Report failed[%d]", iRet);
            }
        }

        if (QueryReportCheck(&SHOULD_REPORT_IP, counter, uiLastReportIpCounter, &uiReportIpFailCounter))
        {
            iRet = ReportAgentIPToServer(this->pcAgentCfg);
            if (iRet)
            {
                uiLastReportIpCounter = counter;
                uiReportIpFailCounter += 1;
            }
            else
            {
                SHOULD_REPORT_IP = 0;
            }
        }

        if (SHOULD_REFRESH_CONF)
        {
            GetLocalAgentConfig(this);
            SHOULD_REFRESH_CONF = 0;
        }

        sleep(1);
        counter ++;
    }
    return AGENT_OK;
}

// Thread即将启动, 通知ThreadHandler做好准备.
INT32 FlowManager_C::PreStartHandler()
{
    INT32 iRet = AGENT_OK;
    iRet = SetNewInterval(pcAgentCfg->GetPollingTimerPeriod());
    if (iRet)
    {
        FLOW_MANAGER_ERROR("SetNewInterval failed[%d], Interval[%d]", iRet, pcAgentCfg->GetPollingTimerPeriod());
        return AGENT_E_PARA;
    }
    return iRet;
}

// Thread即将停止, 通知ThreadHandler主动退出.
INT32 FlowManager_C::PreStopHandler()
{
    SetNewInterval(0);
    return AGENT_OK;
}

void FlowManager_C::FlowManagerAction(UINT32 action)
{
    SetNewInterval(action);
    if (START_AGENT == action)
    {
        StartThread();
    }
}

void FlowManager_C::SetPkgFlag()
{
    vector<AgentFlowTableEntry_S>::iterator pAgentFlowEntry;
    for(pAgentFlowEntry = AgentFlowTable.begin(); pAgentFlowEntry != AgentFlowTable.end();
        pAgentFlowEntry++)
    {
        pAgentFlowEntry->stFlowKey.uiIsBigPkg = !pAgentFlowEntry->stFlowKey.uiIsBigPkg;
    }
}
