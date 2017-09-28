//
// Created by zy on 17-9-16.
//

#ifndef PNTL_AGENT_MESSAGEPLATFORMCLIENT_H
#define PNTL_AGENT_MESSAGEPLATFORMCLIENT_H


extern INT32 ReportDataToServer(ServerAntAgentCfg_C *pcAgentCfg, stringstream *pstrReportData, string strUrl);

// 上报AgentIP至Server
extern INT32 ReportAgentIPToServer(ServerAntAgentCfg_C *pcAgentCfg);
extern bool IS_DEBUG;
#endif //PNTL_AGENT_MESSAGEPLATFORMCLIENT_H
