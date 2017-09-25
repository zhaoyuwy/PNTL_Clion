//
// Created by zy on 17-9-15.
//

#ifndef PNTL_AGENT_DETECTWORKER_H
#define PNTL_AGENT_DETECTWORKER_H

#include <pthread.h>
#include <vector>
#include <netinet/in.h>
#include "ServerAntAgentCfg.h"

#include "Sal.h"
#include "ThreadClass.h"

#define SAL_INADDR_ANY INADDR_ANY

// dscp暂用6bit空间
#define AGENT_MAX_DSCP_VALUE ((1L<<6) - 1)

// ServerAnt定义的拓扑信息
typedef struct tagServerTopo {
    UINT32 uiSvid;             // the source ID. DeviceID(Level == 1) RegionID(Level >1)
    UINT32 uiDvid;             // the destination ID. DeviceID(Level == 1) RegionID(Level >1)
    UINT32 uiLevel;            // 1:Device, 2:Pod, 3:DC, 4:AZ ...

    // 重载运算符, 方便进行key比较.
    bool operator==(const tagServerTopo &other) const {
        if ((other.uiSvid == uiSvid)
            && (other.uiDvid == uiDvid)
            && (other.uiLevel == uiLevel)
                )
            return AGENT_TRUE;
        else
            return AGENT_FALSE;
    }

    bool operator!=(const tagServerTopo &other) const {
        if ((other.uiSvid != uiSvid)
            || (other.uiDvid != uiDvid)
            || (other.uiLevel != uiLevel)
                )
            return AGENT_TRUE;
        else
            return AGENT_FALSE;
    }
} ServerTopo_S;

/* 流表索引信息(key), 根据以下信息可以唯一的识别一条流 */
typedef struct tagFlowKey {
    UINT32 uiUrgentFlow;       // 流优先级.
    AgentDetectProtocolType_E eProtocol;// 协议类型, 见AgentDetectProtocolType_E,添加会话时根据协议类型决定是否与DetectWorker属性进行比较.
    UINT32 uiSrcIP;            // 探测源IP, 添加会话时根据协议类型决定是否与DetectWorker属性进行比较.
    UINT32 uiDestIP;           // 探测目的IP. 添加会话时根据协议类型决定是否与DetectWorker属性进行比较.
    UINT32 uiSrcPort;          // 探测源端口号. 添加会话时根据协议类型决定是否与DetectWorker属性进行比较.
    UINT32 uiDestPort;         // 探测目的端口号.添加会话时根据协议类型决定是否与DetectWorker属性进行比较.
    UINT32 uiDscp;             // 探测报文的DSCP.
    UINT32 uiIsBigPkg;        // 是否大包

    ServerTopo_S stServerTopo;          // ServerAnt定义的拓扑信息, 来源于Server, Agent不做处理, 但是作为key的一部分.

    UINT32 uiAgentFlowTableIndex; // 当前flow在agent Flow Table中的索引, 将flow加入流表时生成, 加速AgentFlowTable查表速度.

    // 重载运算符, 方便进行key比较.
    bool operator==(const tagFlowKey &other) const {
        if ((other.uiAgentFlowTableIndex == uiAgentFlowTableIndex)
            && (other.uiUrgentFlow == uiUrgentFlow)
            && (other.eProtocol == eProtocol)
            && (other.uiSrcIP == uiSrcIP)
            && (other.uiDestIP == uiDestIP)
            && (other.uiSrcPort == uiSrcPort)
            && (other.uiDestPort == uiDestPort)
            && (other.uiDscp == uiDscp)
            && (other.stServerTopo == stServerTopo)
                )
            return AGENT_TRUE;
        else
            return AGENT_FALSE;
    }

    bool operator!=(const tagFlowKey &other) const {
        if ((other.uiAgentFlowTableIndex != uiAgentFlowTableIndex)
            || (other.uiUrgentFlow != uiUrgentFlow)
            || (other.eProtocol != eProtocol)
            || (other.uiSrcIP != uiSrcIP)
            || (other.uiDestIP != uiDestIP)
            || (other.uiSrcPort != uiSrcPort)
            || (other.uiDestPort != uiDestPort)
            || (other.uiDscp != uiDscp)
            || (other.stServerTopo != stServerTopo)
                )
            return AGENT_TRUE;
        else
            return AGENT_FALSE;
    }
} FlowKey_S;

// DetectWorkerSession_S.uiSessionState 会话状态机
enum {
    SESSION_STATE_INITED = 1,    // 完成初始化,待发送探测报文.
    SESSION_STATE_SEND_FAIELD,    // 报文发送失败.
    SESSION_STATE_WAITING_REPLY,  // 得待应答报文.
    SESSION_STATE_WAITING_CHECK,  // 已经收到应答报文,待查询结果.
    SESSION_STATE_TIMEOUT,        // 会话应答报文超时.
    SESSION_STATE_MAX
};

// worker角色:sender(Client side), target(Server side)
enum {
    WORKER_ROLE_CLIENT = 0,    // sender(Client side) 发送探测报文
    WORKER_ROLE_SERVER,         // target(Server side) 响应探测报文,发送应答报文
    WORKER_ROLE_MAX
};

// 时间戳信息. 不直接使用timeval是因为字节序转换接口(htonl)当前只支持32位INT32.
typedef struct tagPacketTime {
    UINT32 uiSec;           // 秒
    UINT32 uiUsec;          // 微秒

    // 重载运算符, 方便进行时延计算
    INT64 operator-(const tagPacketTime &other) const {
        return (INT64) uiUsec - (INT64) (other.uiUsec) + ((INT64) uiSec - (INT64) (other.uiSec)) * SECOND_USEC;
    }
} PacketTime_S;

// 探测报文格式 修改该结构时务必同步修改PacketHtoN()和PacketNtoH()函数
typedef struct tagPacketInfo {
    UINT32 uiSequenceNumber;    // 报文序列号,sender发送报文的时候生成.
    UINT32 uiRole;
    PacketTime_S stT1;               // sender发出报文的时间
    PacketTime_S stT2;               // target收到报文的时间
    PacketTime_S stT3;               // target发出应答报文的时间
    PacketTime_S stT4;               // sender收到应答报文的时间
} PacketInfo_S;

// DetectWorker 配置信息
typedef struct tagWorkerCfg {
    AgentDetectProtocolType_E eProtocol;    // 协议类型, 见AgentDetectProtocolType_E, 创建socket时使用.
    UINT32 uiRole;

    UINT32 uiListenPort;                // 探测源IP, 创建socket时使用.
    UINT32 uiSrcIP;                // 探测源IP, 创建socket时使用.
    UINT32 uiDestIP;               // 探测目的IP. 预留TCP扩展
    UINT32 uiSrcPort;              // 探测源端口号. 创建socket时使用
    UINT32 uiDestPort;             // 探测目的端口号. 预留TCP扩展.
} WorkerCfg_S;
struct IcmpEchoReply {
    int icmpSeq;
    int icmpLen;
    int ipTtl;
    double rtt;
    std::string fromAddr;
    bool isReply;
};
struct PingResult {
    int dataLen;
    int nsend;
    int nreceived;
    std::string ip;
    std::string error;
    std::vector<IcmpEchoReply> icmpEchoReplys;
};
// DetectWorker 会话信息
typedef struct tagDetectWorkerSession {
    /* 会话管理 */
    UINT32 uiSessionState;     // 会话状态机: 待发送探测报文, 等待应答报文, 已收到应答报文, 报文超时.
    UINT32 uiSequenceNumber;   // 本会话的序列号.

    /* 流信息6元组 */
    FlowKey_S stFlowKey;

    /* 探测结果 */
    PacketTime_S stT1;                //时间戳信息, 探测报文从Sender出发时间.
    PacketTime_S stT2;                //时间戳信息, 探测报文到达Target的时间.
    PacketTime_S stT3;                //时间戳信息, 应答报文从Target出发的时间.
    PacketTime_S stT4;                //时间戳信息, 应答报文到达Sender的时间.
} DetectWorkerSession_S;
#define PACKET_SIZE     4096

// DetectWorker类定义,负责探测报文发送和接收.
class DetectWorker_C : ThreadClass_C {
private:
    /*  */

    WorkerCfg_S stCfg;                              // 当前Worker配置的探测协议.
    UINT32 uiSequenceNumber;                // 本Worker的当前序列号,起始值为随机数.
    INT32 InitCfg(WorkerCfg_S stNewWorker);           // 初始化stCfg, 由Init()触发

    vector <DetectWorkerSession_S> SessionList;     // 会话列表, 保存尚未完成探测会话.
    sal_mutex_t WorkerSessionLock;                  // 会话互斥锁, 保护SessionList


    INT32 WorkerSocket;                               // 当前Worker使用的Socket.
    INT32 ReleaseSocket();                            // 释放socket资源
    INT32 InitSocket();                               // 根据stProtocol信息申请socket资源.
    INT32 GetSocket();                                // 获取当前socket
    INT32 TxPacket(DetectWorkerSession_S *
    pNewSession);               // 启动报文发送.PushSession()时触发.
    INT32 TxUpdateSession(DetectWorkerSession_S *
    pNewSession);               // 报文发送完成后, 刷新会话状态.

    /* Thread 实现代码 */
    INT32 ThreadHandler();                            // 任务主处理函数
    INT32 PreStopHandler();                           // StopThread触发, 通知ThreadHandler主动退出.
    INT32 PreStartHandler();                          // StartThread触发, 通知ThreadHandler即将被调用.

    UINT32 uiHandlerDefaultInterval;          // Handler状态刷新默认周期, 单位为us
    INT32 RxUpdateSession
            (PacketInfo_S *pstPakcet);                 // Rx任务收到应答报文后, 通知worker刷新会话列表, Rx任务使用
    struct timeval tvSub(struct timeval timeval1, struct timeval timeval2);

public:
    DetectWorker_C();                               // 构造函数, 填充默认值.
    ~DetectWorker_C();                              // 析构函数, 释放必要资源.
    ServerAntAgentCfg_C *pcAgentCfg;                   // agent_cfg
    INT32 Init(WorkerCfg_S stNewWorker, ServerAntAgentCfg_C *pcNewAgentCfg);         // 根据入参完成对象初始化, FlowManage使用.
    INT32 PushSession(FlowKey_S stNewFlow);           // 添加探测任务, FlowManage使用.
    INT32 PopSession(DetectWorkerSession_S *
    pOldSession);               // 查询探测结果, FlowManage使用.

    int packIcmp(int pack_no, struct icmp *icmp);

    unsigned short getChksum(unsigned short *addr, int len);

    bool unpackIcmp(char *buf, int len, struct IcmpEchoReply *icmpEchoReply);

    INT32 m_nsend;
    INT32 m_icmp_seq;

    char m_sendpacket[PACKET_SIZE];
    char m_recvpacket[PACKET_SIZE];
    int m_maxPacketSize;
    int m_sockfd;
    int m_datalen;
    int m_nreceived;
    struct sockaddr_in m_dest_addr;
    struct sockaddr_in m_from_addr;
    pid_t m_pid;
};

#endif //PNTL_AGENT_DETECTWORKER_H
