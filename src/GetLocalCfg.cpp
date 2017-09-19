//
// Created by zy on 17-9-15.
//
#include <fstream>

using namespace std;

#include "Sal.h"
#include "Log.h"
#include "AgentJsonAPI.h"
#include "GetLocalCfg.h"
#include "ServerAntAgentCfg.h"
#include "MessagePlatformClient.h"

#define SERVER_ANT_CFG_FILE_NAME "ServerAntAgent.cfg"
#define SERVER_ANT_CFG_FILE_PATH "/etc/ServerAntAgent/"

const string AGENT_CONFIG_FILE_NAME = "agentConfig.cfg";
const string AGENT_CONFIG_FILE_PATH = "/opt/huawei/ServerAntAgent";

string GetJsonDataFromFile(string sFileName, string sFilePath)
{

    INT32  iRet = AGENT_OK;
    stringstream ssCfgFileName;
    ifstream ifsAgentCfg;

    ssCfgFileName << sFileName;

    ifsAgentCfg.open(ssCfgFileName.str().c_str());
    if ( ifsAgentCfg.fail() )
    {
        INIT_INFO("No cfg file[%s] in current dir, trying [%s] ...", sFileName.c_str(), sFilePath.c_str());

        ssCfgFileName.clear();
        ssCfgFileName.str("");
        ssCfgFileName << sFilePath << sFileName;
        ifsAgentCfg.open(ssCfgFileName.str().c_str());
        if ( ifsAgentCfg.fail() )
        {
            INIT_ERROR("Can't open cfg file[%s]", ssCfgFileName.str().c_str());
            return "";
        }
    }

    INIT_INFO("Using cfg file[%s]", ssCfgFileName.str().c_str());

    string strCfgJsonData((istreambuf_iterator<char>(ifsAgentCfg)),  (istreambuf_iterator<char>()));

    ifsAgentCfg.close();

    return strCfgJsonData;

}

INT32 GetLocalCfg(ServerAntAgentCfg_C * pcCfg)
{
    INT32  iRet = AGENT_OK;
    string strCfgJsonData;
    stringstream ssCfgFileName;

    strCfgJsonData = GetJsonDataFromFile(SERVER_ANT_CFG_FILE_NAME, SERVER_ANT_CFG_FILE_PATH);
    if ("" == strCfgJsonData)
    {
        return AGENT_E_ERROR;
    }

    iRet = ParserLocalCfg(strCfgJsonData.c_str(), pcCfg);
    if (iRet)
    {
        INIT_ERROR("ParserLocalCfg failed[%d]", iRet);
        return AGENT_E_ERROR;
    }

    return AGENT_OK;
}

INT32 GetLocalAgentConfig(FlowManager_C * pcFlowManager)
{

    INT32  iRet = AGENT_OK;
    string strCfgJsonData;
    stringstream ssCfgFileName;

    strCfgJsonData = GetJsonDataFromFile(AGENT_CONFIG_FILE_NAME, AGENT_CONFIG_FILE_PATH);
    if ("" == strCfgJsonData)
    {
        return AGENT_E_ERROR;
    }

    iRet = ParseLocalAgentConfig(strCfgJsonData.c_str(), pcFlowManager);
    if (iRet)
    {
        INIT_ERROR("ParserLocalCfg failed[%d]", iRet);
        return AGENT_E_ERROR;
    }

    return AGENT_OK;
}

UINT32 GetProbePeriod(FlowManager_C * pcFlowManager)
{
    string strCfgJsonData;
    stringstream ssCfgFileName;
    strCfgJsonData = GetJsonDataFromFile(AGENT_CONFIG_FILE_NAME, AGENT_CONFIG_FILE_PATH);
    if ("" == strCfgJsonData)
    {
        return AGENT_E_ERROR;
    }

    UINT32 uiRet = ParseProbePeriodConfig(strCfgJsonData.c_str(), pcFlowManager);
    return uiRet;
}
