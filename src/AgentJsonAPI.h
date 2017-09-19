//
// Created by zy on 17-9-15.
//

#ifndef PNTL_AGENT_AGENTJSONAPI_H
#define PNTL_AGENT_AGENTJSONAPI_H
#include <boost/property_tree/json_parser.hpp>

using namespace std;
// 使用boost的property_tree扩展库处理json格式数据.
using namespace boost::property_tree;
#include <sstream>
#include "FlowManager.h"

// 解析Agent本地配置文件, 完成初始化配置.
extern INT32 ParserLocalCfg(const CHAR * pcJsonData, ServerAntAgentCfg_C * pcCfg);

// 生成json格式的字符串, 用于向Analyzer上报延时信息.
extern INT32 CreateLatencyReportData(AgentFlowTableEntry_S * pstAgentFlowEntry, stringstream * pssReportData, UINT32 maxDelay, UINT32 bigPkgSize);

// 生成json格式的字符串, 用于向Analyzer上报丢包信息.
extern INT32 CreateDropReportData(AgentFlowTableEntry_S * pstAgentFlowEntry, stringstream * pssReportData, UINT32 bigPkgSize);

extern INT32 CreatAgentIPRequestPostData(ServerAntAgentCfg_C * pcCfg, stringstream * pssPostData);

extern INT32 ParseLocalAgentConfig(const char * pcJsonData, FlowManager_C * pcFlowManager);

extern INT32 GetFlowInfoFromConfigFile(string dip, ServerFlowKey_S * pstNewServerFlowKey, ServerAntAgentCfg_C* pcAgentCfg);

extern UINT32 ParseProbePeriodConfig(const char * pcJsonData, FlowManager_C * pcFlowManager);

#endif //PNTL_AGENT_AGENTJSONAPI_H
