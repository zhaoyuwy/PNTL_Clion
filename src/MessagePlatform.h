//
// Created by zy on 17-9-15.
//

#ifndef PNTL_AGENT_MESSAGEPLATFORM_H
#define PNTL_AGENT_MESSAGEPLATFORM_H


#include "ServerAntAgentCfg.h"

// 向Server注册Agent
extern INT32 ReportToServer(ServerAntAgentCfg_C *pcCfg);

// 获取Server端配置信息
extern INT32 GetCfgFromServer(ServerAntAgentCfg_C *pcCfg);


#endif //PNTL_AGENT_MESSAGEPLATFORM_H
