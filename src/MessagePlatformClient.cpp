//
// Created by zy on 17-9-16.
//


#include <curl/curl.h>
#include <sstream>
#include <stdlib.h>
#include <iostream>
using namespace std;

#include "Log.h"
#include "AgentJsonAPI.h"
#include "ServerAntAgentCfg.h"
#include "MessagePlatformClient.h"

// http 请求处理超时时间,单位s, 避免因为Server未响应数据导致挂死
#define AGENT_REQUEST_TIMEOUT 30
std::string GetTokenString();
std::string getPostToken();
size_t ReceiveResponce(void *ptr, size_t size, size_t nmemb, stringstream *pssResponce)
{
    char * pStr = (char *)ptr;
    if (strlen(pStr) != (nmemb + 2))
    {
        MSG_CLIENT_WARNING("ReceiveResponce wrong pStr size is [%u]", strlen(pStr));
    }

    char* newPStr = (char*)malloc(size * nmemb + 1);
    if (NULL == newPStr)
    {
        MSG_CLIENT_ERROR("Apply for size [%d] memory fail.", size * nmemb + 1);
        return 0;
    }
    sal_memset(newPStr, 0, size * nmemb + 1);
    memcpy(newPStr, pStr, size * nmemb);
    (*pssResponce) << newPStr;
    MSG_CLIENT_WARNING("ReceiveResponce..................strlen[%d].........................data:'%s'         nmemb:%d",
                       strlen(newPStr), newPStr, nmemb);
    free(newPStr);

    return size * nmemb;
}

const CHAR* ACCEPT_TYPE = "Accept:application/json";
const CHAR* CONTENT_TYPE = "Content-Type:application/json";


// 通过http post操作提交数据
INT32 HttpPostData(stringstream * pssUrl, stringstream * pssPostData, stringstream *pssResponceData)
{
    INT32 iRet = AGENT_OK;
    string strTemp;

    CURL *curl;
    CURLcode res;
    char error_msg[CURL_ERROR_SIZE];
    struct curl_slist *headers = NULL;
    curl_global_init(CURL_GLOBAL_ALL);

    // 创建一个curl句柄
    curl = curl_easy_init();
    if (curl)
    {
        // 开始配置curl句柄.
        // 可选打印详细信息, 帮助定位.
        res = curl_easy_setopt(curl, CURLOPT_VERBOSE, AGENT_FALSE);
        if(CURLE_OK != res)
        {
            MSG_CLIENT_WARNING("curl easy setopt CURLOPT_VERBOSE failed[%d][%s]", res, curl_easy_strerror(res));
        }

        // 配置curl出现错误时提供详细错误信息
        sal_memset(error_msg, 0, sizeof(error_msg));
        res = curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_msg);
        if(CURLE_OK != res)
        {
            MSG_CLIENT_WARNING("curl easy setopt CURLOPT_ERRORBUFFER failed[%d][%s]", res, curl_easy_strerror(res));
        }


        string tokenstring ="Authorization:Bearer "+GetTokenString();

        headers = curl_slist_append(headers, tokenstring.c_str());
        headers = curl_slist_append(headers, "user_name:c00000000");
        headers = curl_slist_append(headers, "service_name:pntl");
        headers = curl_slist_append(headers, "Content-Type:application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl,CURLOPT_SSL_VERIFYPEER,0);
        // 设定超时时间, 避免因为SERVER无响应而挂死.
        res = curl_easy_setopt(curl, CURLOPT_TIMEOUT, AGENT_REQUEST_TIMEOUT);
        if(CURLE_OK != res)
        {
            MSG_CLIENT_ERROR("curl easy setopt CURLOPT_TIMEOUT failed[%d][%s],detail info[%s]", res, curl_easy_strerror(res), error_msg);
            curl_easy_cleanup(curl);
            curl_global_cleanup();
            return AGENT_E_ERROR;
        }

        // http中 "\n" 有特殊含义, 字符串中的换行符全部要替换掉.
        string strOld = "\n";
        string strNew = " ";

        // 设定URL
        strTemp.clear();
        strTemp = pssUrl->str();
        sal_string_replace(&strTemp, &strOld, &strNew);
        MSG_CLIENT_INFO("URL:[%s]", strTemp.c_str());
//        res = curl_easy_setopt(curl, CURLOPT_URL, "\"https://cas.lf.hwclouds.com:8243/Kafka/v1/mq/pntl\"");
        res = curl_easy_setopt(curl, CURLOPT_URL, strTemp.c_str());
        cout <<"###################################"<<strTemp.c_str()<<endl;

        if(CURLE_OK != res)
        {
            MSG_CLIENT_ERROR("curl easy setopt CURLOPT_URL failed[%d][%s],detail info[%s]", res, curl_easy_strerror(res), error_msg);
            curl_easy_cleanup(curl);
            curl_slist_free_all(headers);
            curl_global_cleanup();
            return AGENT_E_ERROR;
        }

        // 设定 post 数据
        // 直接使用ssPostData.str().c_str()时, 字符串中有空字符, 使用string转一下.
        strTemp.clear();
        strTemp = pssPostData->str();
        sal_string_replace(&strTemp, &strOld, &strNew);

        MSG_CLIENT_INFO("PostFields:[%s]", strTemp.c_str());
        res = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, strTemp.c_str());
        if(CURLE_OK != res)
        {
            MSG_CLIENT_ERROR("curl easy setopt CURLOPT_POSTFIELDS failed[%d][%s],detail info[%s]", res, curl_easy_strerror(res), error_msg);
            curl_easy_cleanup(curl);
            curl_slist_free_all(headers);
            curl_global_cleanup();
            return AGENT_E_ERROR;
        }

        // 设定接收response的函数
        res = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ReceiveResponce);
        if(CURLE_OK != res)
        {
            MSG_CLIENT_ERROR("curl easy setopt CURLOPT_POSTFIELDS failed[%d][%s],detail info[%s]", res, curl_easy_strerror(res), error_msg);
            curl_easy_cleanup(curl);
            curl_slist_free_all(headers);
            curl_global_cleanup();
            return AGENT_E_ERROR;
        }
        pssResponceData->clear();
        pssResponceData->str("");
        // 设定ReceiveResponce函数的参数
        res = curl_easy_setopt(curl, CURLOPT_WRITEDATA, pssResponceData);
        if(CURLE_OK != res)
        {
            MSG_CLIENT_ERROR("curl easy setopt CURLOPT_POSTFIELDS failed[%d][%s],detail info[%s]", res, curl_easy_strerror(res), error_msg);
            curl_easy_cleanup(curl);
            curl_slist_free_all(headers);
            curl_global_cleanup();
            return AGENT_E_ERROR;
        }

        // 执行提交操作, 建立连接,提交数据,等待应答等.
        res = curl_easy_perform(curl);
        if(CURLE_OK != res)
        {
            MSG_CLIENT_ERROR("curl easy perform failed[%d][%s],detail info[%s]", res, curl_easy_strerror(res), error_msg);
            curl_easy_cleanup(curl);
            curl_slist_free_all(headers);
            curl_global_cleanup();
            return AGENT_E_ERROR;
        }
        // 释放curl资源, 断开连接.
        curl_easy_cleanup(curl);
    }
    else
    {
        MSG_CLIENT_ERROR("curl easy init failed");
        iRet = AGENT_E_ERROR;
    }
    curl_slist_free_all(headers);
    curl_global_cleanup();
    return iRet;
}

const string HTTP_PREFIX = "https://";
const string BASIC_TOKEN_PREFIX = "basic ";
const string COLON = ":";

// 向ServerAnrServer请求新的probe列表
INT32 ReportDataToServer(ServerAntAgentCfg_C *pcAgentCfg, stringstream * pstrReportData, string strUrl)
{
    INT32 iRet = AGENT_OK;

    // 用于提交的URL地址
    stringstream ssUrl;
    // 保存post的response(查询结果),json格式字符串, 后续交给json模块处理.
    stringstream ssResponceData;

    const char * pcJsonData = NULL;

    ssUrl.clear();
    ssUrl << HTTP_PREFIX << pcAgentCfg->GetKafkaIp() << strUrl;

    stringstream ssBasicToken;
    ssBasicToken.clear();
    ssBasicToken<< pcAgentCfg->GetKafkaBasicToken();

    cout<<"######################################## basicToken" << ssBasicToken<<endl;
    ssResponceData.clear();
    ssResponceData.str("");
    iRet = HttpPostData(&ssUrl, pstrReportData, &ssResponceData);
    if (iRet)
    {
        MSG_CLIENT_ERROR("Http Post Data failed[%d]", iRet);
        return iRet;
    }

    // 字符串格式转换.
    string strResponceData = ssResponceData.str();
    pcJsonData = strResponceData.c_str();

    // 处理response数据
    MSG_CLIENT_INFO("Responce [%s]", strResponceData.c_str());

    return iRet;
}

INT32 ReportAgentIPToServer(ServerAntAgentCfg_C * pcAgentCfg)
{
    INT32 iRet = AGENT_OK;

    // 用于提交的URL地址
    stringstream ssUrl;
    // 保存需要post的数据,json格式字符串, 由json模块生成.
    stringstream ssPostData;
    // 保存post的response(查询结果),json格式字符串, 后续交给json模块处理.
    stringstream ssResponceData;
    char * pcJsonData = NULL;

    // 生成 URL
    UINT32 uiServerIP = 0;
    UINT32 uiServerPort = 0;
    pcAgentCfg->GetServerAddress(&uiServerIP, &uiServerPort);

    ssUrl.clear();
    ssUrl << HTTP_PREFIX << pcAgentCfg->GetKafkaIp() << KAFKA_TOPIC_URL << pcAgentCfg->GetTopic();
    MSG_CLIENT_INFO("URL [%s]", ssUrl.str().c_str());


    // 生成post数据
    ssPostData.clear();
    ssPostData.str("");
    iRet = CreatAgentIPRequestPostData(pcAgentCfg, &ssPostData);
    if (iRet)
    {
        MSG_CLIENT_ERROR("Creat Report Agent IP Request Post Data failed[%d]", iRet);
        return iRet;
    }
    MSG_CLIENT_INFO("PostData [%s]", ssPostData.str().c_str());

    ssResponceData.clear();
    ssResponceData.str("");
    iRet = HttpPostData(&ssUrl, &ssPostData, &ssResponceData);
    if (iRet)
    {
        MSG_CLIENT_ERROR("Http Post Data failed[%d]", iRet);
        return iRet;
    }

    // 字符串格式转换.
    string strResponceData = ssResponceData.str();
    pcJsonData = (char *)strResponceData.c_str();

    // 处理response数据
    MSG_CLIENT_INFO("Responce [%s]", strResponceData.c_str());
    return iRet;
}



size_t CurlWrite_CallbackFunc_StdString(void *contents, size_t size, size_t nmemb, std::string *s)
{
    size_t newLength = size*nmemb;
    size_t oldLength = s->size();
    try
    {
        s->resize(oldLength + newLength);
    }
    catch(std::bad_alloc &e)
    {
        //handle memory problem
        return 0;
    }

    std::copy((char*)contents,(char*)contents+newLength,s->begin()+oldLength);
    return size*nmemb;
}

std::string getPostToken()
{
    CURL *curl;
    CURLcode res;

    struct curl_slist *headers = NULL;

    headers = curl_slist_append(headers, "Authorization:basic NWVfeG9fOThOTlhEYUZ0RmU1STFPbkVmX3QwYTo4RWYwVGRwazA3aEFBTnNzR1RaZm1zOHAwVEVh");
    headers = curl_slist_append(headers, "user_name:c00000000");
    headers = curl_slist_append(headers, "service_name:pntl");
    headers = curl_slist_append(headers, "Content-Type:application/x-www-form-urlencoded");

    curl = curl_easy_init();
    std::string s_post_return;
    char error_msg[CURL_ERROR_SIZE];
    if(curl)
    {
        res= curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        res = curl_easy_setopt(curl,CURLOPT_POSTFIELDS,"grant_type=client_credentials");

        res= curl_easy_setopt(curl,CURLOPT_URL,"https://cas.lf.hwclouds.com:8243/token");
        res= curl_easy_setopt(curl,CURLOPT_SSL_VERIFYPEER,0);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWrite_CallbackFunc_StdString);
        res = curl_easy_setopt(curl,CURLOPT_WRITEDATA,&s_post_return);
//        curl_easy_setopt (curl, CURLOPT_VERBOSE, 1L); //remove this to disable verbose output


cout<<"$$$$$$$$$$$$$$$$$$$$$$$$$$$$$"<<endl;
        res = curl_easy_perform(curl);
cout<<"$$$$$$$$$$$$$$$$$$$$$$$$$$$$$"<<endl;

        if(CURLE_OK != res)
        {
            MSG_CLIENT_ERROR("curl easy perform failed[%d][%s],detail info[%s]", res, curl_easy_strerror(res), error_msg);
            curl_easy_cleanup(curl);
            curl_slist_free_all(headers);
            curl_global_cleanup();
            return "error";
        }

        curl_easy_cleanup(curl);
    }
    return s_post_return;

}


string GetTokenString(){
    istringstream iss;

    std::string s_post_return;

    s_post_return = getPostToken();
    iss.str(s_post_return);

    ptree item;
    read_json(iss,item);

    string n = item.get<string>("access_token");
    return n;
}