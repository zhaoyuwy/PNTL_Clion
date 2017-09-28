//
// Created by zy on 17-9-15.
//

#ifndef PNTL_AGENT_AGENTCOMMON_H_H
#define PNTL_AGENT_AGENTCOMMON_H_H

#include "Sal.h"

enum {
    AGENT_OK = 0,
    AGENT_E_ERROR = -1,
    AGENT_E_PARA = -2,
    AGENT_E_NOT_FOUND = -3,
    AGENT_E_MEMORY = -4,
    AGENT_E_TIMER = -5,
    AGENT_E_SOCKET = -6,
    AGENT_E_THREAD = -7,
    AGENT_E_HANDLER = -8,
    AGENT_E_EXIST = -9,
    AGENT_EXIT = -10,
    AGENT_FILTER_DELAY = -11
};

#define AGENT_ENABLE                         1
#define AGENT_DISABLE                        0

#define AGENT_TRUE                          1
#define AGENT_FALSE                         0
#define KAFKA_TOPIC_URL  "/Kafka/v1/mq/"

// Whether agent begin to query pingList
extern UINT32 SHOULD_DETECT_REPORT;

extern UINT32 SHOULD_REPORT_IP;

extern UINT32 SHOULD_REFRESH_CONF;

const UINT32 BIG_PACKAGE_SIZE = 1000;
const UINT32 NORMAL_PACKAGE_SIZE = 40;

const UINT32 START_AGENT = 100000;

const UINT32 STOP_AGENT = 0;

typedef enum tagAgentDetectProtocolType {
    AGENT_DETECT_PROTOCOL_NULL = 0,   // 未配置
    AGENT_DETECT_PROTOCOL_ICMP,       // ICMP,更改支持  201709281556
    AGENT_DETECT_PROTOCOL_UDP,        // UDP, 支持
    AGENT_DETECT_PROTOCOL_TCP,        // TCP,暂不支持
    AGENT_DETECT_PROTOCOL_MAX
} AgentDetectProtocolType_E;
#endif //PNTL_AGENT_AGENTCOMMON_H_H
