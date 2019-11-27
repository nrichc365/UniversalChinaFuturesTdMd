#define MarketQuotationAPI_EXP
#define KLINESTORAGE
//#define HQDATAFILE
//#define KDATAFILE
#define CTP_TRADEAPI
//#define KSV6T_TRADEAPI
#define TRADINGHOURS           10    //交易时间
#define FIRSTSECOND        3600*21   //K线开始时间点
#define BARNUMBER_1MIN     600       //1分钟K线根数
#define BARNUMBER_3MIN     200       //3分钟K线根数
#define BARNUMBER_5MIN     120       //5分钟K线根数
#define BARNUMBER_10MIN    60        //10分钟K线根数
#define BARNUMBER_15MIN    40        //15分钟K线根数
#define BARNUMBER_30MIN    20        //20分钟K线根数
#define BARNUMBER_60MIN    10        //60分钟K线根数
#define TRADINGTIMELINETYPE 1        //时间轴数量
#define TRADINGTIMELINEMIN  1095      //时间轴上时间点数量
#define LOGGER_NAME "MarketQuotationAPI"
#include "MarketQuotationAPI.h"
#include <iostream>
#include <fstream>
#include <array>
#include <math.h>

axapi::MarketQuotationAPI::MarketQuotationAPI(void)
{
    if (initializeLog() == 0)
    {
        //LOG4CPLUS_FATAL(m_root, "initialize LOG OK");
        LOG4CPLUS_FATAL(m_objLogger, "initialize LOG OK");
    }
    else
    {
        LOG4CPLUS_FATAL(m_objLogger, "initialize LOG ERROR");
    }
    m_nRecievedDataCount = 0;
    m_nDCETimeDiff = 0;
    m_nCZCETimeDiff = 0;
    m_nSHFETimeDiff = 0;
    m_nCFFEXTimeDiff = 0;
    m_nIMETimeDiff = 0;
    m_pUserApi = APINamespace CThostFtdcMdApi::CreateFtdcMdApi("MarketQuotationLog\\");
    initializeConnection("4500", "21001", "gtax@1", "tcp://10.6.7.80:41213");
}

axapi::MarketQuotationAPI::MarketQuotationAPI(
    APINamespace TThostFtdcBrokerIDType in_strBrokerID,
    APINamespace TThostFtdcUserIDType in_strUserID,
    APINamespace TThostFtdcPasswordType in_strPasswd,
    char *in_strFrontAddr)
{
    if (initializeLog() == 0)
    {
        //LOG4CPLUS_FATAL(m_root, "initialize LOG OK");
        LOG4CPLUS_FATAL(m_objLogger, "initialize LOG OK");
    }
    else
    {
        LOG4CPLUS_FATAL(m_objLogger, "initialize LOG ERROR");
    }
    m_nRecievedDataCount = 0;
    m_nDCETimeDiff = 0;
    m_nCZCETimeDiff = 0;
    m_nSHFETimeDiff = 0;
    m_nCFFEXTimeDiff = 0;
    m_nIMETimeDiff = 0;
    m_pUserApi = APINamespace CThostFtdcMdApi::CreateFtdcMdApi();
#ifdef KLINESTORAGE
    initiateTradingTimeLine(NULL, 0);
#endif KLINESTORAGE
#ifdef HQDATAFILE
    initializeHQDataIntoFiles();
#endif HQDATAFILE
    initializeConnection(in_strBrokerID, in_strUserID, in_strPasswd, in_strFrontAddr);
}

axapi::MarketQuotationAPI::~MarketQuotationAPI(void)
{
    m_hashMarketDataList.clear();
#ifdef KLINESTORAGE
    for (unsigned int i = 0; i < m_array1mKLine.size(); i++)
    {
        free(m_array1mKLine[i]);
        free(m_array3mKLine[i]);
        free(m_array5mKLine[i]);
        free(m_array10mKLine[i]);
        free(m_array15mKLine[i]);
        free(m_array30mKLine[i]);
        free(m_array60mKLine[i]);
    }
    for (int i = 0; i < TRADINGTIMELINETYPE; i++)
    {
        free(m_TradingTimeLine[i]);
    }
#endif KLINESTORAGE
    m_pUserApi->Release();
    Sleep(WAITFORCLOSEFILE);
    /*CThostFtdcUserLogoutField ReqUserLogoutField;
    memset(&ReqUserLogoutField, 0, sizeof(ReqUserLogoutField));
    strcpy(ReqUserLogoutField.BrokerID, m_chBrokerID);
    strcpy(ReqUserLogoutField.UserID, m_chUserID);
    m_pUserApi->ReqUserLogout(&ReqUserLogoutField, m_nRequestID++);
    WaitForSingleObject(m_hInitEvent, INFINITE);
    m_pUserApi->Join();*/
}


void axapi::MarketQuotationAPI::OnFrontConnected()
{
    char *t_strLogFuncName = "axapi::MarketQuotationAPI::OnFrontConnected";
    char t_strLog[500];
    sprintf_s(t_strLog, "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    sprintf_s(t_strLog, "%s:%s", t_strLogFuncName, "前置连接成功");
    LOG4CPLUS_INFO(m_objLogger, t_strLog);

    setAPIStatus(APISTATUS_CONNECTED);
    SetEvent(m_hInitEvent);
}

void axapi::MarketQuotationAPI::OnFrontDisconnected(
    int in_nReason)
{
    char *t_strLogFuncName = "axapi::MarketQuotationAPI::OnFrontDisconnected";
    char t_strLog[500];
    sprintf_s(t_strLog, "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    sprintf_s(t_strLog, "%s:%s", t_strLogFuncName, "连接中断");
    LOG4CPLUS_FATAL(m_objLogger, t_strLog);

    setAPIStatus(APISTATUS_DISCONNECTED);
}

void axapi::MarketQuotationAPI::OnHeartBeatWarning(
    int in_nTimeLapse)
{
    char *t_strLogFuncName = "axapi::MarketQuotationAPI::OnHeartBeatWarning";
    char t_strLog[500];
    sprintf_s(t_strLog, "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    sprintf_s(t_strLog, "%s:%d%s", t_strLogFuncName, in_nTimeLapse, "分钟未收到报文");
    LOG4CPLUS_WARN(m_objLogger, t_strLog);
}

void axapi::MarketQuotationAPI::OnRspUserLogin(
    APINamespace CThostFtdcRspUserLoginField *pRspUserLogin,
    APINamespace CThostFtdcRspInfoField *pRspInfo,
    int nRequestID,
    bool bIsLast)
{
    char *t_strLogFuncName = "axapi::MarketQuotationAPI::OnRspUserLogin";
    char t_strLog[500];
    try {
        sprintf_s(t_strLog, "%s", t_strLogFuncName);
        LOG4CPLUS_TRACE(m_objLogger, t_strLog);

        double t_dblocaltime = getLocalTime();
        if (pRspInfo->ErrorID == 0 && pRspUserLogin != NULL) {
            sprintf_s(t_strLog, "%s:%s%s", t_strLogFuncName, "响应 | 登录成功...当前交易日:", pRspUserLogin->TradingDay);
            LOG4CPLUS_INFO(m_objLogger, t_strLog);
            m_nCFFEXTimeDiff = getTimebyFormat(pRspUserLogin->FFEXTime) - t_dblocaltime;
            sprintf_s(t_strLog, "%s:%s%f%s%f%c%s%s%d", t_strLogFuncName, "响应 | 本地时间", t_dblocaltime, "与交易所CFFEX时间", getTimebyFormat(pRspUserLogin->FFEXTime), '(', pRspUserLogin->FFEXTime, ")相差", m_nCFFEXTimeDiff);
            LOG4CPLUS_INFO(m_objLogger, t_strLog);
            m_nCZCETimeDiff = getTimebyFormat(pRspUserLogin->CZCETime) - t_dblocaltime;
            sprintf_s(t_strLog, "%s:%s%f%s%f%c%s%s%d", t_strLogFuncName, "响应 | 本地时间", t_dblocaltime, "与交易所CZCE时间", getTimebyFormat(pRspUserLogin->CZCETime), '(', pRspUserLogin->CZCETime, ")相差", m_nCZCETimeDiff);
            LOG4CPLUS_INFO(m_objLogger, t_strLog);
            m_nDCETimeDiff = getTimebyFormat(pRspUserLogin->DCETime) - t_dblocaltime;
            sprintf_s(t_strLog, "%s:%s%f%s%f%c%s%s%d", t_strLogFuncName, "响应 | 本地时间", t_dblocaltime, "与交易所DCE时间", getTimebyFormat(pRspUserLogin->DCETime), '(', pRspUserLogin->DCETime, ")相差", m_nDCETimeDiff);
            LOG4CPLUS_INFO(m_objLogger, t_strLog);
            m_nIMETimeDiff = getTimebyFormat(pRspUserLogin->SHFETime) - t_dblocaltime;
            sprintf_s(t_strLog, "%s:%s%f%s%f%c%s%s%d", t_strLogFuncName, "响应 | 本地时间", t_dblocaltime, "与交易所IME时间", getTimebyFormat(pRspUserLogin->SHFETime), '(', pRspUserLogin->SHFETime, ")相差", m_nIMETimeDiff);
            LOG4CPLUS_INFO(m_objLogger, t_strLog);
            m_nSHFETimeDiff = getTimebyFormat(pRspUserLogin->SHFETime) - t_dblocaltime;
            sprintf_s(t_strLog, "%s:%s%f%s%f%c%s%s%d", t_strLogFuncName, "响应 | 本地时间", t_dblocaltime, "与交易所SHFE时间", getTimebyFormat(pRspUserLogin->SHFETime), '(', pRspUserLogin->SHFETime, ")相差", m_nSHFETimeDiff);
            LOG4CPLUS_INFO(m_objLogger, t_strLog);
        }

        if (pRspInfo->ErrorID != 0)
        {
            sprintf_s(t_strLog, "%s:%s%d%s", t_strLogFuncName, " 响应 | 登录错误...", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
            LOG4CPLUS_FATAL(m_objLogger, t_strLog);
        }
        if (bIsLast) SetEvent(m_hInitEvent);
    }
    catch (std::exception e)
    {
        sprintf_s(t_strLog, "%s", e.what());
        LOG4CPLUS_FATAL(m_objLogger, t_strLog);
    }
}

void axapi::MarketQuotationAPI::OnRspUserLogout(
    APINamespace CThostFtdcUserLogoutField *pUserLogout,
    APINamespace CThostFtdcRspInfoField *pRspInfo,
    int nRequestID,
    bool bIsLast)
{
    char *t_strLogFuncName = "axapi::MarketQuotationAPI::OnRspUserLogout";
    char t_strLog[500];
    sprintf_s(t_strLog, "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    sprintf_s(t_strLog, "%s:%s", t_strLogFuncName, "用户登出");
    LOG4CPLUS_FATAL(m_objLogger, t_strLog);
}

void axapi::MarketQuotationAPI::OnRspError(
    APINamespace CThostFtdcRspInfoField *pRspInfo,
    int nRequestID,
    bool bIsLast)
{
    char *t_strLogFuncName = "axapi::MarketQuotationAPI::OnRspError";
    char t_strLog[500];
    sprintf_s(t_strLog, "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    if ((pRspInfo) && (pRspInfo->ErrorID != 0))
    {
        sprintf_s(t_strLog, "%s:%s%s", t_strLogFuncName, "响应", pRspInfo->ErrorMsg);
        LOG4CPLUS_FATAL(m_objLogger, t_strLog);
    }
}

void axapi::MarketQuotationAPI::OnRspSubMarketData(
    APINamespace CThostFtdcSpecificInstrumentField *pSpecificInstrument,
    APINamespace CThostFtdcRspInfoField *pRspInfo,
    int nRequestID,
    bool bIsLast)
{
    char *t_strLogFuncName = "axapi::MarketQuotationAPI::OnRspSubMarketData";
    char t_strLog[500];
    sprintf_s(t_strLog, "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    if (bIsLast)
    {
        sprintf_s(t_strLog, "%s:%s订阅行情成功", t_strLogFuncName, pSpecificInstrument->InstrumentID);
        LOG4CPLUS_INFO(m_objLogger, t_strLog);
    }
    else
    {
        sprintf_s(t_strLog, "%s:%s订阅行情失败:%d.%s", t_strLogFuncName, pSpecificInstrument->InstrumentID, pRspInfo->ErrorID, pRspInfo->ErrorMsg);
        LOG4CPLUS_ERROR(m_objLogger, t_strLog);
    }
}

void axapi::MarketQuotationAPI::OnRspUnSubMarketData(
    APINamespace CThostFtdcSpecificInstrumentField *pSpecificInstrument,
    APINamespace CThostFtdcRspInfoField *pRspInfo,
    int nRequestID,
    bool bIsLast)
{
    char *t_strLogFuncName = "axapi::MarketQuotationAPI::OnRspUnSubMarketData";
    char t_strLog[500];
    sprintf_s(t_strLog, "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    if (bIsLast)
    {
        sprintf_s(t_strLog, "%s:%s退订行情成功", t_strLogFuncName, pSpecificInstrument->InstrumentID);
        LOG4CPLUS_INFO(m_objLogger, t_strLog);
    }
    else
    {
        sprintf_s(t_strLog, "%s:%s退订行情失败:%d.%s", t_strLogFuncName, pSpecificInstrument->InstrumentID, pRspInfo->ErrorID, pRspInfo->ErrorMsg);
        LOG4CPLUS_ERROR(m_objLogger, t_strLog);
    }
}

void axapi::MarketQuotationAPI::OnRtnDepthMarketData(
    APINamespace CThostFtdcDepthMarketDataField *pDepthMarketData)
{
    char *t_strLogFuncName = "axapi::MarketQuotationAPI::OnRtnDepthMarketData";
    char t_strLog[500];
    sprintf_s(t_strLog, "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    try
    {
        //处理特殊数据
        if (pDepthMarketData->PreOpenInterest == DBL_MAX) pDepthMarketData->PreOpenInterest = -1;
        if (pDepthMarketData->PreClosePrice == DBL_MAX) pDepthMarketData->PreClosePrice = -1;
        if (pDepthMarketData->PreSettlementPrice == DBL_MAX) pDepthMarketData->PreSettlementPrice = -1;
        if (pDepthMarketData->LastPrice == DBL_MAX) pDepthMarketData->LastPrice = -1;
        if (pDepthMarketData->Volume == INT_MAX) pDepthMarketData->Volume = -1;
        if (pDepthMarketData->Turnover == DBL_MAX) pDepthMarketData->Turnover = -1;
        if (pDepthMarketData->OpenInterest == DBL_MAX) pDepthMarketData->OpenInterest = -1;
        if (pDepthMarketData->OpenPrice == DBL_MAX) pDepthMarketData->OpenPrice = -1;
        if (pDepthMarketData->ClosePrice == DBL_MAX) pDepthMarketData->ClosePrice = -1;
        if (pDepthMarketData->HighestPrice == DBL_MAX) pDepthMarketData->HighestPrice = -1;
        if (pDepthMarketData->LowestPrice == DBL_MAX) pDepthMarketData->LowestPrice = -1;
        if (pDepthMarketData->SettlementPrice == DBL_MAX) pDepthMarketData->SettlementPrice = -1;
        if (pDepthMarketData->LowerLimitPrice == DBL_MAX) pDepthMarketData->LowerLimitPrice = -1;
        if (pDepthMarketData->UpperLimitPrice == DBL_MAX) pDepthMarketData->UpperLimitPrice = -1;
        if (pDepthMarketData->PreDelta == DBL_MAX) pDepthMarketData->PreDelta = -1;
        if (pDepthMarketData->CurrDelta == DBL_MAX) pDepthMarketData->CurrDelta = -1;
        if (pDepthMarketData->BidPrice1 == DBL_MAX) pDepthMarketData->BidPrice1 = -1;
        if (pDepthMarketData->BidVolume1 == DBL_MAX) pDepthMarketData->BidVolume1 = -1;
        if (pDepthMarketData->AskPrice1 == DBL_MAX) pDepthMarketData->AskPrice1 = -1;
        if (pDepthMarketData->AskVolume1 == DBL_MAX) pDepthMarketData->AskVolume1 = -1;
        if (pDepthMarketData->BidPrice2 == DBL_MAX) pDepthMarketData->BidPrice2 = -1;
        if (pDepthMarketData->BidVolume2 == DBL_MAX) pDepthMarketData->BidVolume2 = -1;
        if (pDepthMarketData->AskPrice2 == DBL_MAX) pDepthMarketData->AskPrice2 = -1;
        if (pDepthMarketData->AskVolume2 == DBL_MAX) pDepthMarketData->AskVolume2 = -1;
        if (pDepthMarketData->BidPrice3 == DBL_MAX) pDepthMarketData->BidPrice3 = -1;
        if (pDepthMarketData->BidVolume3 == DBL_MAX) pDepthMarketData->BidVolume3 = -1;
        if (pDepthMarketData->AskPrice3 == DBL_MAX) pDepthMarketData->AskPrice3 = -1;
        if (pDepthMarketData->AskVolume3 == DBL_MAX) pDepthMarketData->AskVolume3 = -1;
        if (pDepthMarketData->BidPrice4 == DBL_MAX) pDepthMarketData->BidPrice4 = -1;
        if (pDepthMarketData->BidVolume4 == DBL_MAX) pDepthMarketData->BidVolume4 = -1;
        if (pDepthMarketData->AskPrice4 == DBL_MAX) pDepthMarketData->AskPrice4 = -1;
        if (pDepthMarketData->AskVolume4 == DBL_MAX) pDepthMarketData->AskVolume4 = -1;
        if (pDepthMarketData->BidPrice5 == DBL_MAX) pDepthMarketData->BidPrice5 = -1;
        if (pDepthMarketData->BidVolume5 == DBL_MAX) pDepthMarketData->BidVolume5 = -1;
        if (pDepthMarketData->AskPrice5 == DBL_MAX) pDepthMarketData->AskPrice5 = -1;
        if (pDepthMarketData->AskVolume5 == DBL_MAX) pDepthMarketData->AskVolume5 = -1;
        if (pDepthMarketData->AveragePrice == DBL_MAX) pDepthMarketData->AveragePrice = -1;
        //std::cout<<"bef:"<<t_strMarketData<<std::endl;
        sprintf_s(t_strLog, "%s:%s%zd", t_strLogFuncName, "m_hashMarketDataList当前含有用户数bef:", m_hashMarketDataList.size());
        LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
        /*hash_map<string, struct MarketDataField>::iterator t_iterator;
        for(t_iterator=m_hashMarketDataList.begin();t_iterator!=m_hashMarketDataList.end();t_iterator++)
        {
        cout << "key:" << t_iterator->first << "|" << t_iterator->second.InstrumentID << "," << t_iterator->second.LastPrice << endl;
        }*/
        if (m_hashMarketDataList.find(pDepthMarketData->InstrumentID) != m_hashMarketDataList.end())
        {
            sprintf_s(t_strLog, "%s:%s%s %f", t_strLogFuncName, "开始赋值", pDepthMarketData->InstrumentID, pDepthMarketData->LastPrice);
            LOG4CPLUS_TRACE(m_objLogger, t_strLog);
            strcpy_s(m_hashMarketDataList[pDepthMarketData->InstrumentID].TradingDay, sizeof(m_hashMarketDataList[pDepthMarketData->InstrumentID].TradingDay), pDepthMarketData->TradingDay);
            strcpy_s(m_hashMarketDataList[pDepthMarketData->InstrumentID].ActionDay, sizeof(m_hashMarketDataList[pDepthMarketData->InstrumentID].ActionDay), pDepthMarketData->ActionDay);
            strcpy_s(m_hashMarketDataList[pDepthMarketData->InstrumentID].InstrumentID, sizeof(m_hashMarketDataList[pDepthMarketData->InstrumentID].InstrumentID), pDepthMarketData->InstrumentID);
            strcpy_s(m_hashMarketDataList[pDepthMarketData->InstrumentID].ExchangeID, sizeof(m_hashMarketDataList[pDepthMarketData->InstrumentID].ExchangeID), pDepthMarketData->ExchangeID);
            strcpy_s(m_hashMarketDataList[pDepthMarketData->InstrumentID].ExchangeInstID, sizeof(m_hashMarketDataList[pDepthMarketData->InstrumentID].ExchangeInstID), pDepthMarketData->ExchangeInstID);
            m_hashMarketDataList[pDepthMarketData->InstrumentID].PreOpenInterest = pDepthMarketData->PreOpenInterest;
            m_hashMarketDataList[pDepthMarketData->InstrumentID].PreClosePrice = pDepthMarketData->PreClosePrice;
            m_hashMarketDataList[pDepthMarketData->InstrumentID].PreSettlementPrice = pDepthMarketData->PreSettlementPrice;
            m_hashMarketDataList[pDepthMarketData->InstrumentID].LastPrice = pDepthMarketData->LastPrice;
            m_hashMarketDataList[pDepthMarketData->InstrumentID].Volume = pDepthMarketData->Volume;
            m_hashMarketDataList[pDepthMarketData->InstrumentID].Turnover = pDepthMarketData->Turnover;
            m_hashMarketDataList[pDepthMarketData->InstrumentID].OpenInterest = pDepthMarketData->OpenInterest;
            m_hashMarketDataList[pDepthMarketData->InstrumentID].OpenPrice = pDepthMarketData->OpenPrice;
            m_hashMarketDataList[pDepthMarketData->InstrumentID].ClosePrice = pDepthMarketData->ClosePrice;
            m_hashMarketDataList[pDepthMarketData->InstrumentID].HighestPrice = pDepthMarketData->HighestPrice;
            m_hashMarketDataList[pDepthMarketData->InstrumentID].LowestPrice = pDepthMarketData->LowestPrice;
            m_hashMarketDataList[pDepthMarketData->InstrumentID].SettlementPrice = pDepthMarketData->SettlementPrice;
            m_hashMarketDataList[pDepthMarketData->InstrumentID].LowerLimitPrice = pDepthMarketData->LowerLimitPrice;
            m_hashMarketDataList[pDepthMarketData->InstrumentID].UpperLimitPrice = pDepthMarketData->UpperLimitPrice;
            m_hashMarketDataList[pDepthMarketData->InstrumentID].PreDelta = pDepthMarketData->PreDelta;
            m_hashMarketDataList[pDepthMarketData->InstrumentID].CurrDelta = pDepthMarketData->CurrDelta;
            strcpy_s(m_hashMarketDataList[pDepthMarketData->InstrumentID].UpdateTime, sizeof(m_hashMarketDataList[pDepthMarketData->InstrumentID].UpdateTime), pDepthMarketData->UpdateTime);
            m_hashMarketDataList[pDepthMarketData->InstrumentID].UpdateMillisec = pDepthMarketData->UpdateMillisec;
            m_hashMarketDataList[pDepthMarketData->InstrumentID].BidPrice1 = pDepthMarketData->BidPrice1;
            m_hashMarketDataList[pDepthMarketData->InstrumentID].BidVolume1 = pDepthMarketData->BidVolume1;
            m_hashMarketDataList[pDepthMarketData->InstrumentID].AskPrice1 = pDepthMarketData->AskPrice1;
            m_hashMarketDataList[pDepthMarketData->InstrumentID].AskVolume1 = pDepthMarketData->AskVolume1;
            m_hashMarketDataList[pDepthMarketData->InstrumentID].BidPrice2 = pDepthMarketData->BidPrice2;
            m_hashMarketDataList[pDepthMarketData->InstrumentID].BidVolume2 = pDepthMarketData->BidVolume2;
            m_hashMarketDataList[pDepthMarketData->InstrumentID].AskPrice2 = pDepthMarketData->AskPrice2;
            m_hashMarketDataList[pDepthMarketData->InstrumentID].AskVolume2 = pDepthMarketData->AskVolume2;
            m_hashMarketDataList[pDepthMarketData->InstrumentID].BidPrice3 = pDepthMarketData->BidPrice3;
            m_hashMarketDataList[pDepthMarketData->InstrumentID].BidVolume3 = pDepthMarketData->BidVolume3;
            m_hashMarketDataList[pDepthMarketData->InstrumentID].AskPrice3 = pDepthMarketData->AskPrice3;
            m_hashMarketDataList[pDepthMarketData->InstrumentID].AskVolume3 = pDepthMarketData->AskVolume3;
            m_hashMarketDataList[pDepthMarketData->InstrumentID].BidPrice4 = pDepthMarketData->BidPrice4;
            m_hashMarketDataList[pDepthMarketData->InstrumentID].BidVolume4 = pDepthMarketData->BidVolume4;
            m_hashMarketDataList[pDepthMarketData->InstrumentID].AskPrice4 = pDepthMarketData->AskPrice4;
            m_hashMarketDataList[pDepthMarketData->InstrumentID].AskVolume4 = pDepthMarketData->AskVolume4;
            m_hashMarketDataList[pDepthMarketData->InstrumentID].BidPrice5 = pDepthMarketData->BidPrice5;
            m_hashMarketDataList[pDepthMarketData->InstrumentID].BidVolume5 = pDepthMarketData->BidVolume5;
            m_hashMarketDataList[pDepthMarketData->InstrumentID].AskPrice5 = pDepthMarketData->AskPrice5;
            m_hashMarketDataList[pDepthMarketData->InstrumentID].AskVolume5 = pDepthMarketData->AskVolume5;
            m_hashMarketDataList[pDepthMarketData->InstrumentID].AveragePrice = pDepthMarketData->AveragePrice;
        }
        sprintf_s(t_strLog, "%s:%s%zd", t_strLogFuncName, "m_hashMarketDataList当前含有用户数aft:", m_hashMarketDataList.size());
        LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
        /*for(t_iterator=m_hashMarketDataList.begin();t_iterator!=m_hashMarketDataList.end();t_iterator++)
        {
        cout << "key:" << t_iterator->first << "|" << t_iterator->second.InstrumentID << "," << t_iterator->second.LastPrice << endl;
        }*/
        writeHQData(pDepthMarketData);
#ifdef KLINESTORAGE
        recordKData(pDepthMarketData->InstrumentID);
#endif KLINESTORAGE
    }
    catch (std::exception e)
    {
        sprintf_s(t_strLog, "%s:%s", t_strLogFuncName, e.what());
        LOG4CPLUS_FATAL(m_objLogger, t_strLog);
    }
}

/// 初始化日志文件
int axapi::MarketQuotationAPI::initializeLog()
{
    char *t_strLogFuncName = "axapi::MarketQuotationAPI::initializeLog";
    log4cplus::initialize();
    log4cplus::helpers::LogLog::getLogLog()->setInternalDebugging(true);
    m_root = log4cplus::Logger::getRoot();
    m_objLogger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT(LOGGER_NAME));
    try {
        log4cplus::ConfigureAndWatchThread configureThread(
            LOG4CPLUS_TEXT("log4cplus.properties"), 5 * 1000);
    }
    catch (std::exception e) {
        LOG4CPLUS_FATAL(m_root, "initialLog exception");
    }
    return 0;
}

/// 链接行情服务器
int axapi::MarketQuotationAPI::initializeConnection(
    APINamespace TThostFtdcBrokerIDType in_strBrokerID,
    APINamespace TThostFtdcUserIDType in_strUserID,
    APINamespace TThostFtdcPasswordType in_strPasswd,
    char *in_strFrontAddr)
{
    char *t_strLogFuncName = "axapi::MarketQuotationAPI::initializeConnection";
    char t_strLog[500];
    sprintf_s(t_strLog, "%s:%s", t_strLogFuncName, in_strFrontAddr);
    LOG4CPLUS_INFO(m_objLogger, t_strLog);

    strcpy_s(m_strBrokerID, sizeof(m_strBrokerID), in_strBrokerID);
    strcpy_s(m_strUserID, sizeof(m_strUserID), in_strUserID);
    strcpy_s(m_strPassword, sizeof(m_strPassword), in_strPasswd);
    m_hInitEvent = CreateEvent(NULL, true, false, NULL);
    m_nRequestID = 0;

    APINamespace CThostFtdcReqUserLoginField ReqUserLoginField;
    memset(&ReqUserLoginField, '\0', sizeof(ReqUserLoginField));
    strcpy_s(ReqUserLoginField.BrokerID, sizeof(ReqUserLoginField.BrokerID), m_strBrokerID);
    strcpy_s(ReqUserLoginField.UserID, sizeof(ReqUserLoginField.UserID), m_strUserID);
    strcpy_s(ReqUserLoginField.Password, sizeof(ReqUserLoginField.Password), m_strPassword);
    //strcpy(ReqUserLoginField.UserProductInfo, "");
    //strcpy(ReqUserLoginField.InterfaceProductInfo, "");
    //std::cout << ReqUserLoginField.BrokerID << ',' << ReqUserLoginField.UserID << ',' << ReqUserLoginField.Password << ',' << std::endl;

    sprintf_s(t_strLog, "%s:Connecting...", t_strLogFuncName);
    LOG4CPLUS_WARN(m_objLogger, t_strLog);
    ResetEvent(m_hInitEvent);
    m_pUserApi->RegisterSpi(this);
    m_pUserApi->RegisterFront(in_strFrontAddr);
    m_pUserApi->Init();
    WaitForSingleObject(m_hInitEvent, INFINITE);
    ResetEvent(m_hInitEvent);
    int returncode = m_pUserApi->ReqUserLogin(&ReqUserLoginField, m_nRequestID++);
    WaitForSingleObject(m_hInitEvent, INFINITE);

    /*cout << "开始订阅行情..." << endl;
    while(returncode == 0)
    {
    char* t_instrumentidlist[] = {
    "au1812","PM1901","sn1809","SM1810","i1811","OI1811","AP1812","p1803","j1901","c1901","ni1810","jd1810",
    "sn1803","au1803","ag1802","ag1805","i1805","AP1805","m1807","pb1803","T1809","ru1810","pb1810","b1803",
    "jm1809","rb1808","sn1808","SR1801","bu1912","RS1807","MA1809","CF1803","wr1806","wr1807","CF1809",
    "wr1808","wr1812","bb1803","bb1901","RM1901","WH1803","fu1810","FG1806","v1804","v1805","v1807","RS1808",
    "pp1807","al1808","al1810","ag1811","l1806","l1807","l1811","i1809","rb1805","TA1901","p1901","ni1803",
    "cu1806","bu1809","zn1807","SF1806","JR1805","rb1812","CY1803","hc1810","cu1901","JR1803","hc1809","hc1811",
    "IC1806","PM1809","y1901","jm1805","jm1807","jd1807","ag1806","l1809","CY1806","CY1808","CY1811","WH1805",
    "MA1805","IC1803","y1805","v1802","TA1806","wr1804","c1809","wr1811","bb1809","IH1809","bb1811","bb1812",
    "jd1808","OI1805","cs1811","TA1810","m1801","IH1801","ag1807","hc1803","fb1802","hc1804","y1809","SM1805",
    "SM1807","TA1812","p1806","p1807","j1802","rb1807","RI1801","ni1802","ZC1811","cu1807","i1803","cu1808",
    "PM1811","SF1801","SF1804","SR1901","SF1807","SF1808","RM1805","LR1901","IF1803","IF1806","IH1803","IH1806",
    "fb1803","ag1901","bu1812","bu1903","al1804","ag1803","i1806","CF1805","CY1809","CY1810","WH1807","pb1809",
    "b1806","cs1809","rb1804","pb1812","RM1807","rb1802","zn1812","MA1806","RM1808","RM1809","MA1812","TA1803",
    "TA1805","ZC1803","wr1803","WH1901","JR1811","SR1903","i1808","ZC1809","FG1810","SM1901","jd1806","pb1806",
    "ag1812","hc1802","sn1806","sn1807","MA1901","RM1811","l1901","l1812","a1803","zn1804","sn1812","b1809",
    "ni1804","ni1805","a1805","cu1810","c1811","PM1807","i1812","ni1901","cs1901","hc1806","bu1803","rb1901",
    "AP1810","ag1808","ag1809","ag1810","jm1803","al1801","al1802","al1805","ag1804","CY1804","sn1810","TF1806",
    "b1808","FG1808","ru1811","TA1802","TA1807","zn1810","ni1807","rb1809","rb1810","y1807","ZC1805","SR1905",
    "sn1805","fb1804","TA1808","fu1811","FG1803","FG1804","FG1807","pp1804","JR1901","jd1802","RS1811","pp1811",
    "ru1804","jd1812","y1803","ru1808","ru1809","pp1901","MA1804","y1811","cu1802","v1901","LR1811","j1804",
    "ZC1807","j1810","j1811","c1803","cu1803","au1806","FG1901","LR1809","l1802","cu1811","zn1806","i1810",
    "jd1809","IC1809","SF1812","p1808","pb1901","bu1806","SR1805","ZC1901","cs1805","i1901","AP1807","ni1811",
    "CY1805","j1807","CY1807","pb1801","pb1802","j1806","a1809","b1805","b1807","bb1802","zn1811","rb1803",
    "i1804","IC1802","SR1807","bb1804","bb1805","bb1806","b1812","bb1810","cs1803","SR1809","fb1809","fb1811",
    "m1805","FG1809","JR1807","v1810","jm1812","pp1805","SR1811","jd1803","ru1806","ru1801","IC1801","y1808",
    "pp1809","AP1901","pp1812","a1903","hc1801","hc1805","SM1806","y1812","al1901","l1810","p1804","IF1809",
    "zn1801","j1801","LR1807","RI1803","ZC1812","cu1812","l1803","SM1808","CF1901","SF1810","m1808","CY1802",
    "pb1808","bu1807","p1811","c1805","TF1809","ni1812","WH1809","au1802","bu1802","jm1802","al1803","i1807",
    "l1808","a1907","TF1803","hc1812","i1802","hc1901","OI1807","wr1805","CF1811","wr1809","wr1810","b1901",
    "TA1809","fu1807","fb1812","m1803","FG1802","OI1803","jd1804","ru1805","m1901","al1809","pb1807","MA1802",
    "SM1803","p1805","zn1805","j1805","LR1805","j1808","j1809","RS1809","RI1811","cu1804","cu1805","jm1808",
    "ZC1806","l1804","SM1809","SF1805","PM1803","SF1809","zn1901","SF1811","CY1801","p1802","bu1804","bu1805",
    "hc1808","RI1901","IF1802","a1905","T1803","jd1901","au1810","wr1901","pb1804","CY1812","bb1801","IH1802",
    "jm1901","p1812","au1902","MA1807","v1808","MA1811","bu1906","bu1909","ZC1802","OI1809","RI1807","b1810",
    "b1811","fb1805","fb1807","fu1812","FG1805","ZC1808","FG1811","v1806","v1809","pp1803","v1812","pp1806",
    "al1806","al1807","ru1807","ru1803","pp1810","AP1811","al1811","pb1805","a1901","TA1811","c1807","l1805",
    "zn1802","sn1901","j1803","CF1807","cs1807","rb1806","sn1811","j1812","RI1805","ZC1810","ni1806","OI1901",
    "jm1810","jm1811","m1811","cu1809","SF1802","SF1803","PM1805","m1812","m1809","hc1807","p1810","T1806",
    "ni1808","ni1809","jd1811","au1804","au1808","jm1804","jm1806","a1807","CY1901","al1812","a1811","b1801",
    "pb1811","fb1901","rb1811","TA1804","SR1803","ZC1801","MA1808","ZC1804","WH1811","SR1907","sn1804","RM1803",
    "bb1807","bb1808","MA1810","fb1806","fu1806","fb1808","SF1901","fb1810","FG1801","FG1812","v1803","pp1802",
    "v1811","jd1805","pp1808","SM1804","MA1803","SM1811","SM1812","ru1901","p1809","zn1803","zn1808","zn1809"};
    cout << t_instrumentidlist << endl;
    if (m_pUserApi->SubscribeMarketData(t_instrumentidlist,503) == 0)
    break;
    }*/
    return 0;
}

#ifdef KLINESTORAGE
/// 初始化K线时间线数据
void axapi::MarketQuotationAPI::initiateTradingTimeLine(char **FilenameList, int FileNum)
{
    char *t_strLogFuncName = "axapi::MarketQuotationAPI::initiateTradingTimeLine";
    char t_strLog[500];

    /*
    * 时间轴地址分配为时间轴数据量+1，下标从1开始，下标为0保存当前时间
    */
    m_TradingTimeLine = (TradingMinuteField**)malloc(sizeof(TradingMinuteField*)*TRADINGTIMELINETYPE);
    for (int t_nTimeLineType = 0; t_nTimeLineType < TRADINGTIMELINETYPE; t_nTimeLineType++)
    {
        TradingMinuteField *t_TradingTimeLine = (TradingMinuteField*)malloc(sizeof(TradingMinuteField)*(TRADINGTIMELINEMIN + 1));
        memset(t_TradingTimeLine, '\0', sizeof(TradingMinuteField)*(TRADINGTIMELINEMIN + 1));
        m_TradingTimeLine[t_nTimeLineType] = t_TradingTimeLine;

        /*
        * 通过加载数据文件，获取时间线数据
        */
        std::fstream t_getTradingTimeLine;
        t_getTradingTimeLine.open("TradingMinuteLine.dat", std::ios_base::in);
        //int t_nLineCounts;
        if (t_getTradingTimeLine.is_open())
        {
            //t_nLineCounts = 1;
            //std::vector<std::array<char,20>> t_linetext;
            std::array<char, 20> t_text;
            for (int t_nLineCounts = 1; !t_getTradingTimeLine.eof() && t_nLineCounts < (TRADINGTIMELINEMIN + 1); t_nLineCounts++)
            {
                for (int i = 0; i <= 9; i++)
                {
                    t_getTradingTimeLine.getline(&t_text[0], 200, ',');
                    switch (i)
                    {
                    case 0:
                    {
                        t_TradingTimeLine[t_nLineCounts].CurrentMinute = atoi(&t_text[0]);
                        t_TradingTimeLine[t_nLineCounts].OffsetValue = t_nLineCounts;
                        break;
                    }
                    case 1: t_TradingTimeLine[t_nLineCounts].isTradeFlag = atoi(&t_text[0]); break;
                    case 2: strcpy_s(t_TradingTimeLine[t_nLineCounts].TradingDay, sizeof(t_TradingTimeLine[t_nLineCounts].TradingDay), &t_text[0]); break;
                    case 3: t_TradingTimeLine[t_nLineCounts].BarSerials_1Min = atoi(&t_text[0]); break;
                    case 4: t_TradingTimeLine[t_nLineCounts].BarSerials_3Min = atoi(&t_text[0]); break;
                    case 5: t_TradingTimeLine[t_nLineCounts].BarSerials_5Min = atoi(&t_text[0]); break;
                    case 6: t_TradingTimeLine[t_nLineCounts].BarSerials_10Min = atoi(&t_text[0]); break;
                    case 7: t_TradingTimeLine[t_nLineCounts].BarSerials_15Min = atoi(&t_text[0]); break;
                    case 8: t_TradingTimeLine[t_nLineCounts].BarSerials_30Min = atoi(&t_text[0]); break;
                    case 9: t_TradingTimeLine[t_nLineCounts].BarSerials_60Min = atoi(&t_text[0]); break;
                    }
                }
                //t_nLineCounts++;
            }
            t_getTradingTimeLine.close();
        }
        else
        {
            sprintf_s(t_strLog, "%s:%s", t_strLogFuncName, "文件打开异常");
            LOG4CPLUS_ERROR(m_objLogger, t_strLog);
        }
    }
}
#endif KLINESTORAGE

#ifdef KLINESTORAGE
/// 订阅新合约初始化K线存储
int axapi::MarketQuotationAPI::initialKMarketDataSingle(const char *pInstrument)
{
    char *t_strLogFuncName = "axapi::MarketQuotationAPI::initialKMarketDataSingle";
    char t_strLog[500];

    if (findMarketDataIndex(pInstrument) < 0)
    {
        sprintf_s(t_strLog, "%s:%s%s", t_strLogFuncName, "分配K线空间给合约", pInstrument);
        LOG4CPLUS_INFO(m_objLogger, t_strLog);

        /*
        * K线地址分配为当日K线根数+1，K线下标从1开始，下标为0保存当前时间前所有Bar累计值
        */
        Contract t_newInstrument;
        KMarketDataField *t_array1mKLine = (KMarketDataField*)malloc(sizeof(KMarketDataField)*(BARNUMBER_1MIN + 1));
        memset(t_array1mKLine, '\0', sizeof(KMarketDataField)*(BARNUMBER_1MIN + 1));
        KMarketDataField *t_array3mKLine = (KMarketDataField*)malloc(sizeof(KMarketDataField)*(BARNUMBER_3MIN + 1));
        memset(t_array3mKLine, '\0', sizeof(KMarketDataField)*(BARNUMBER_3MIN + 1));
        KMarketDataField *t_array5mKLine = (KMarketDataField*)malloc(sizeof(KMarketDataField)*(BARNUMBER_5MIN + 1));
        memset(t_array5mKLine, '\0', sizeof(KMarketDataField)*(BARNUMBER_5MIN + 1));
        KMarketDataField *t_array10mKLine = (KMarketDataField*)malloc(sizeof(KMarketDataField)*(BARNUMBER_10MIN + 1));
        memset(t_array10mKLine, '\0', sizeof(KMarketDataField)*(BARNUMBER_10MIN + 1));
        KMarketDataField *t_array15mKLine = (KMarketDataField*)malloc(sizeof(KMarketDataField)*(BARNUMBER_15MIN + 1));
        memset(t_array15mKLine, '\0', sizeof(KMarketDataField)*(BARNUMBER_15MIN + 1));
        KMarketDataField *t_array30mKLine = (KMarketDataField*)malloc(sizeof(KMarketDataField)*(BARNUMBER_30MIN + 1));
        memset(t_array30mKLine, '\0', sizeof(KMarketDataField)*(BARNUMBER_30MIN + 1));
        KMarketDataField *t_array60mKLine = (KMarketDataField*)malloc(sizeof(KMarketDataField)*(BARNUMBER_60MIN + 1));
        memset(t_array60mKLine, '\0', sizeof(KMarketDataField)*(BARNUMBER_60MIN + 1));
        strcpy_s(t_newInstrument.InstrumentID, sizeof(APINamespace TThostFtdcInstrumentIDType), pInstrument);
        m_arrayContracts.push_back(t_newInstrument);
        m_array1mKLine.push_back(t_array1mKLine);
        m_array3mKLine.push_back(t_array3mKLine);
        m_array5mKLine.push_back(t_array5mKLine);
        m_array10mKLine.push_back(t_array10mKLine);
        m_array15mKLine.push_back(t_array15mKLine);
        m_array30mKLine.push_back(t_array30mKLine);
        m_array60mKLine.push_back(t_array60mKLine);
        for (int i = 0; i < (BARNUMBER_1MIN + 1); i++)
        {
            strcpy_s(t_array1mKLine[i].TradingDay, sizeof(t_array1mKLine[i].TradingDay), "20181022");
            strcpy_s(t_array1mKLine[i].Contract, sizeof(t_array1mKLine[i].Contract), pInstrument);
            t_array1mKLine[i].BarSerials = i;
            t_array1mKLine[i].SecondsPeriod = 1;
            t_array1mKLine[i].OpenPrice = 0;
            t_array1mKLine[i].ClosePrice = 0;
            t_array1mKLine[i].HighestPrice = 0;
            t_array1mKLine[i].LowestPrice = 0;
            switch (i)
            {
            case 0:
            {
                t_array1mKLine[i].begintime = 0;
                t_array1mKLine[i].endtime = 0;
                break;
            }
            default:
            {
                t_array1mKLine[i].begintime = FIRSTSECOND + i * 60 * 1;
                t_array1mKLine[i].endtime = FIRSTSECOND + i * 60 * (1 * 2 - 1) + 59;
                break;
            }
            }
        }
        for (int i = 0; i < (BARNUMBER_3MIN + 1); i++)
        {
            strcpy_s(t_array3mKLine[i].Contract, sizeof(t_array3mKLine[i].Contract), pInstrument);
            t_array3mKLine[i].BarSerials = i;
            t_array3mKLine[i].SecondsPeriod = 3;
            t_array3mKLine[i].OpenPrice = 0;
            t_array3mKLine[i].ClosePrice = 0;
            t_array3mKLine[i].HighestPrice = 0;
            t_array3mKLine[i].LowestPrice = 0;
            switch (i)
            {
            case 0:
            {
                t_array1mKLine[i].begintime = 0;
                t_array1mKLine[i].endtime = 0;
                break;
            }
            default:
            {
                t_array3mKLine[i].begintime = FIRSTSECOND + i * 60 * 3;
                t_array3mKLine[i].endtime = FIRSTSECOND + i * 60 * (3 * 2 - 1) + 59;
                break;
            }
            }
        }
        for (int i = 0; i < (BARNUMBER_5MIN + 1); i++)
        {
            strcpy_s(t_array5mKLine[i].Contract, sizeof(t_array5mKLine[i].Contract), pInstrument);
            t_array5mKLine[i].BarSerials = i;
            t_array5mKLine[i].SecondsPeriod = 5;
            t_array5mKLine[i].OpenPrice = 0;
            t_array5mKLine[i].ClosePrice = 0;
            t_array5mKLine[i].HighestPrice = 0;
            t_array5mKLine[i].LowestPrice = 0;
            switch (i)
            {
            case 0:
            {
                t_array1mKLine[i].begintime = 0;
                t_array1mKLine[i].endtime = 0;
                break;
            }
            default:
            {
                t_array5mKLine[i].begintime = FIRSTSECOND + i * 60 * 5;
                t_array5mKLine[i].endtime = FIRSTSECOND + i * 60 * (5 * 2 - 1) + 59;
                break;
            }
            }
        }
        for (int i = 0; i < (BARNUMBER_10MIN + 1); i++)
        {
            strcpy_s(t_array10mKLine[i].Contract, sizeof(t_array10mKLine[i].Contract), pInstrument);
            t_array10mKLine[i].BarSerials = i;
            t_array10mKLine[i].SecondsPeriod = 10;
            t_array10mKLine[i].OpenPrice = 0;
            t_array10mKLine[i].ClosePrice = 0;
            t_array10mKLine[i].HighestPrice = 0;
            t_array10mKLine[i].LowestPrice = 0;
            switch (i)
            {
            case 0:
            {
                t_array1mKLine[i].begintime = 0;
                t_array1mKLine[i].endtime = 0;
                break;
            }
            default:
            {
                t_array10mKLine[i].begintime = FIRSTSECOND + i * 60 * 10;
                t_array10mKLine[i].endtime = FIRSTSECOND + i * 60 * (10 * 2 - 1) + 59;
                break;
            }
            }
        }
        for (int i = 0; i < (BARNUMBER_15MIN + 1); i++)
        {
            strcpy_s(t_array15mKLine[i].Contract, sizeof(t_array15mKLine[i].Contract), pInstrument);
            t_array15mKLine[i].BarSerials = i;
            t_array15mKLine[i].SecondsPeriod = 15;
            t_array15mKLine[i].OpenPrice = 0;
            t_array15mKLine[i].ClosePrice = 0;
            t_array15mKLine[i].HighestPrice = 0;
            t_array15mKLine[i].LowestPrice = 0;
            switch (i)
            {
            case 0:
            {
                t_array1mKLine[i].begintime = 0;
                t_array1mKLine[i].endtime = 0;
                break;
            }
            default:
            {
                t_array15mKLine[i].begintime = FIRSTSECOND + i * 60 * 15;
                t_array15mKLine[i].endtime = FIRSTSECOND + i * 60 * (15 * 2 - 1) + 59;
                break;
            }
            }
        }
        for (int i = 0; i < (BARNUMBER_30MIN + 1); i++)
        {
            strcpy_s(t_array30mKLine[i].Contract, sizeof(t_array30mKLine[i].Contract), pInstrument);
            t_array30mKLine[i].BarSerials = i;
            t_array30mKLine[i].SecondsPeriod = 30;
            t_array30mKLine[i].OpenPrice = 0;
            t_array30mKLine[i].ClosePrice = 0;
            t_array30mKLine[i].HighestPrice = 0;
            t_array30mKLine[i].LowestPrice = 0;
            switch (i)
            {
            case 0:
            {
                t_array1mKLine[i].begintime = 0;
                t_array1mKLine[i].endtime = 0;
                break;
            }
            default:
            {
                t_array30mKLine[i].begintime = FIRSTSECOND + i * 60 * 30;
                t_array30mKLine[i].endtime = FIRSTSECOND + i * 60 * (30 * 2 - 1) + 59;
                break;
            }
            }
        }
        for (int i = 0; i < (BARNUMBER_60MIN + 1); i++)
        {
            strcpy_s(t_array60mKLine[i].Contract, sizeof(t_array60mKLine[i].Contract), pInstrument);
            t_array60mKLine[i].BarSerials = i;
            t_array60mKLine[i].SecondsPeriod = 60;
            t_array60mKLine[i].OpenPrice = 0;
            t_array60mKLine[i].ClosePrice = 0;
            t_array60mKLine[i].HighestPrice = 0;
            t_array60mKLine[i].LowestPrice = 0;
            switch (i)
            {
            case 0:
            {
                t_array1mKLine[i].begintime = 0;
                t_array1mKLine[i].endtime = 0;
                break;
            }
            default:
            {
                t_array60mKLine[i].begintime = FIRSTSECOND + i * 60 * 60;
                t_array60mKLine[i].endtime = FIRSTSECOND + i * 60 * (60 * 2 - 1) + 59;
                break;
            }
            }
        }
    }
    return 0;
}
#endif KLINESTORAGE

#ifdef KLINESTORAGE
/// 查找合约是否已订阅，并获取到合约数据索引号
int axapi::MarketQuotationAPI::findMarketDataIndex(const char *pInstrument)
{
    for (unsigned int i = 0; i < m_arrayContracts.size(); i++)
    {
        if (strcmp(pInstrument, m_arrayContracts[i].InstrumentID) == 0)
        {
            return i;
        }
    }
    return -100;
}
#endif KLINESTORAGE

#ifdef KLINESTORAGE
/// 查找合约是否已订阅，并获取到合约数据索引号
int axapi::MarketQuotationAPI::findCurrentTradingTimeLineOffset()
{
    int t_nCurrentMinute = floor(getLocalTime() / 60);
    /// 获得品种对应时间轴,不同品种对应不同时间轴
    int t_iTimeLineTypeIndex = 0;
    /*
    * 如果当前时间轴偏移量已经被检索过则直接使用下标0的缓存数据，如果没有检索过则检索生成缓存数据
    */
    if (m_TradingTimeLine[t_iTimeLineTypeIndex][0].CurrentMinute == t_nCurrentMinute)
    {
        return m_TradingTimeLine[t_iTimeLineTypeIndex][0].OffsetValue;
    }
    else
    {
        for (int i = 1; i < TRADINGTIMELINEMIN + 1; i++)
        {
            if (m_TradingTimeLine[t_iTimeLineTypeIndex][i].CurrentMinute == t_nCurrentMinute)
            {
                m_TradingTimeLine[t_iTimeLineTypeIndex][0].CurrentMinute = m_TradingTimeLine[t_iTimeLineTypeIndex][i].CurrentMinute;
                m_TradingTimeLine[t_iTimeLineTypeIndex][0].OffsetValue = m_TradingTimeLine[t_iTimeLineTypeIndex][i].OffsetValue;
                strcpy_s(m_TradingTimeLine[t_iTimeLineTypeIndex][0].TradingDay, sizeof(m_TradingTimeLine[t_iTimeLineTypeIndex][0].TradingDay), m_TradingTimeLine[t_iTimeLineTypeIndex][i].TradingDay);
                m_TradingTimeLine[t_iTimeLineTypeIndex][0].isTradeFlag = m_TradingTimeLine[t_iTimeLineTypeIndex][i].isTradeFlag;
                m_TradingTimeLine[t_iTimeLineTypeIndex][0].BarSerials_1Min = m_TradingTimeLine[t_iTimeLineTypeIndex][i].BarSerials_1Min;
                m_TradingTimeLine[t_iTimeLineTypeIndex][0].BarSerials_3Min = m_TradingTimeLine[t_iTimeLineTypeIndex][i].BarSerials_3Min;
                m_TradingTimeLine[t_iTimeLineTypeIndex][0].BarSerials_5Min = m_TradingTimeLine[t_iTimeLineTypeIndex][i].BarSerials_5Min;
                m_TradingTimeLine[t_iTimeLineTypeIndex][0].BarSerials_10Min = m_TradingTimeLine[t_iTimeLineTypeIndex][i].BarSerials_10Min;
                m_TradingTimeLine[t_iTimeLineTypeIndex][0].BarSerials_15Min = m_TradingTimeLine[t_iTimeLineTypeIndex][i].BarSerials_15Min;
                m_TradingTimeLine[t_iTimeLineTypeIndex][0].BarSerials_30Min = m_TradingTimeLine[t_iTimeLineTypeIndex][i].BarSerials_30Min;
                m_TradingTimeLine[t_iTimeLineTypeIndex][0].BarSerials_60Min = m_TradingTimeLine[t_iTimeLineTypeIndex][i].BarSerials_60Min;
                return i;
            }
        }
    }
    return -100;
}
#endif KLINESTORAGE

#ifdef KLINESTORAGE
/// 查找当前时间是否属于指定合约交易时间内以及时间轴上位置
int axapi::MarketQuotationAPI::findCurrentTradingTimeLineOffset(const char *pInstrument)
{
    int t_nCurrentMinute = floor(getLocalTime() / 60);
    /// 获得品种对应时间轴,不同品种对应不同时间轴
    int t_iTimeLineTypeIndex = 0;
    /*
    * 如果当前时间轴偏移量已经被检索过则直接使用下标0的缓存数据，如果没有检索过则检索生成缓存数据
    */
    if (m_TradingTimeLine[t_iTimeLineTypeIndex][0].CurrentMinute == t_nCurrentMinute)
    {
        return m_TradingTimeLine[t_iTimeLineTypeIndex][0].OffsetValue;
    }
    else
    {
        for (int i = 1; i < TRADINGTIMELINEMIN + 1; i++)
        {
            if (m_TradingTimeLine[t_iTimeLineTypeIndex][i].CurrentMinute == t_nCurrentMinute)
            {
                m_TradingTimeLine[t_iTimeLineTypeIndex][0].CurrentMinute = m_TradingTimeLine[t_iTimeLineTypeIndex][i].CurrentMinute;
                m_TradingTimeLine[t_iTimeLineTypeIndex][0].OffsetValue = m_TradingTimeLine[t_iTimeLineTypeIndex][i].OffsetValue;
                strcpy_s(m_TradingTimeLine[t_iTimeLineTypeIndex][0].TradingDay, sizeof(m_TradingTimeLine[t_iTimeLineTypeIndex][0].TradingDay), m_TradingTimeLine[t_iTimeLineTypeIndex][i].TradingDay);
                m_TradingTimeLine[t_iTimeLineTypeIndex][0].isTradeFlag = m_TradingTimeLine[t_iTimeLineTypeIndex][i].isTradeFlag;
                m_TradingTimeLine[t_iTimeLineTypeIndex][0].BarSerials_1Min = m_TradingTimeLine[t_iTimeLineTypeIndex][i].BarSerials_1Min;
                m_TradingTimeLine[t_iTimeLineTypeIndex][0].BarSerials_3Min = m_TradingTimeLine[t_iTimeLineTypeIndex][i].BarSerials_3Min;
                m_TradingTimeLine[t_iTimeLineTypeIndex][0].BarSerials_5Min = m_TradingTimeLine[t_iTimeLineTypeIndex][i].BarSerials_5Min;
                m_TradingTimeLine[t_iTimeLineTypeIndex][0].BarSerials_10Min = m_TradingTimeLine[t_iTimeLineTypeIndex][i].BarSerials_10Min;
                m_TradingTimeLine[t_iTimeLineTypeIndex][0].BarSerials_15Min = m_TradingTimeLine[t_iTimeLineTypeIndex][i].BarSerials_15Min;
                m_TradingTimeLine[t_iTimeLineTypeIndex][0].BarSerials_30Min = m_TradingTimeLine[t_iTimeLineTypeIndex][i].BarSerials_30Min;
                m_TradingTimeLine[t_iTimeLineTypeIndex][0].BarSerials_60Min = m_TradingTimeLine[t_iTimeLineTypeIndex][i].BarSerials_60Min;
                return i;
            }
        }
    }
    return -100;
}
#endif KLINESTORAGE

#ifdef KLINESTORAGE
/// 记录K线数据
void axapi::MarketQuotationAPI::recordKData(const char *pInstrument)
{
    char *t_strLogFuncName = "axapi::MarketQuotationAPI::recordKData";
    char t_strLog[500];
    sprintf_s(t_strLog, "%s:%s%s%s", t_strLogFuncName, "开始记录合约", pInstrument, "的K线行情");
    LOG4CPLUS_INFO(m_objLogger, t_strLog);

    /// 获得品种对应时间轴,不同品种对应不同时间轴
    int t_iTimeLineTypeIndex = 0;
    /// 获得当前合约存储位置索引
    int t_iInstrumentIndex = findMarketDataIndex(pInstrument);
    //std::cout << "t_iInstrumentIndex:" << t_iInstrumentIndex << std::endl;
    /// 未找到合约存储索引则停止
    if (t_iInstrumentIndex >= 0)
    {
        /// 获取当前时间存储位置索引
        int t_iTimeLineindex = findCurrentTradingTimeLineOffset(pInstrument);
        //std::cout << "t_iTimeLineindex:" << t_iTimeLineindex << std::endl;

        /// 未找到时间索引则停止
        if (t_iTimeLineindex >= 0)
        {
            /*
            * 获得1分钟K线偏移量，记录K线数据
            * 偏移量从1开始 0记录当前BAR之前所有BAR累计值
            */
#pragma region KBar1m
            int t_i1MinOffset = m_TradingTimeLine[t_iTimeLineTypeIndex][t_iTimeLineindex].BarSerials_1Min;
            /*
            * 保存行情累积值,K线切换时计算,切换标志为当前BAR.closeprice为0,切换后closeprice被lastprice刷新
            */
            if ((m_array1mKLine[t_iInstrumentIndex] + t_i1MinOffset)->ClosePrice == 0)
            {
                // 高开低收为上条BAR的值
                (m_array1mKLine[t_iInstrumentIndex] + 0)->OpenPrice = (m_array1mKLine[t_iInstrumentIndex] + t_i1MinOffset - 1)->OpenPrice;
                //(m_array1mKLine[t_iInstrumentIndex]+0)->OpenPrice==0?(m_array1mKLine[t_iInstrumentIndex]+t_i1MinOffset-1)->OpenPrice:(m_array1mKLine[t_iInstrumentIndex]+0)->OpenPrice;
                (m_array1mKLine[t_iInstrumentIndex] + 0)->HighestPrice = (m_array1mKLine[t_iInstrumentIndex] + t_i1MinOffset - 1)->HighestPrice;
                //(m_array1mKLine[t_iInstrumentIndex]+0)->HighestPrice>(m_array1mKLine[t_iInstrumentIndex]+t_i1MinOffset-1)->HighestPrice?(m_array1mKLine[t_iInstrumentIndex]+0)->HighestPrice:(m_array1mKLine[t_iInstrumentIndex]+t_i1MinOffset-1)->HighestPrice;
                (m_array1mKLine[t_iInstrumentIndex] + 0)->LowestPrice = (m_array1mKLine[t_iInstrumentIndex] + t_i1MinOffset - 1)->LowestPrice;
                //(m_array1mKLine[t_iInstrumentIndex]+0)->LowestPrice <(m_array1mKLine[t_iInstrumentIndex]+t_i1MinOffset-1)->LowestPrice ?(m_array1mKLine[t_iInstrumentIndex]+0)->LowestPrice :(m_array1mKLine[t_iInstrumentIndex]+t_i1MinOffset-1)->LowestPrice;
                (m_array1mKLine[t_iInstrumentIndex] + 0)->ClosePrice = (m_array1mKLine[t_iInstrumentIndex] + t_i1MinOffset - 1)->ClosePrice;
                //(m_array1mKLine[t_iInstrumentIndex]+t_i1MinOffset-1)->ClosePrice;
                // 累积之前所有BAR时的成交量
                (m_array1mKLine[t_iInstrumentIndex] + 0)->Volume += (m_array1mKLine[t_iInstrumentIndex] + t_i1MinOffset - 1)->Volume;
                // 累积之前所有BAR时的成交量
                (m_array1mKLine[t_iInstrumentIndex] + 0)->Turnover += (m_array1mKLine[t_iInstrumentIndex] + t_i1MinOffset - 1)->Turnover;
            }
            /*
            * 保存对应K线BAR
            * 当天第一条无法取上一条BAR的值，需要特殊处理
            */
            // 如果当前BAR.openprice为0，则需要赋值，用上一条BAR.closeprice或者用当天的开盘价
            if ((m_array1mKLine[t_iInstrumentIndex] + t_i1MinOffset)->OpenPrice == 0)
            {
                switch (t_i1MinOffset)
                {
                    // 如果为第一根BAR，则openprice为当天行情开盘价，该数据在当前行情数据中
                case 1:(m_array1mKLine[t_iInstrumentIndex] + t_i1MinOffset)->OpenPrice = m_hashMarketDataList[pInstrument].OpenPrice;
                    break;
                    /*
                    * 如果为其他BAR，则用上一条BAR的收盘价作为开盘价
                    * 但如果BAR记录不全，即前面缺失，则用当天开盘价作为openprice
                    */
                default:(m_array1mKLine[t_iInstrumentIndex] + t_i1MinOffset)->OpenPrice
                    = (m_array1mKLine[t_iInstrumentIndex] + t_i1MinOffset - 1)->ClosePrice == 0 ? m_hashMarketDataList[pInstrument].OpenPrice : (m_array1mKLine[t_iInstrumentIndex] + t_i1MinOffset - 1)->ClosePrice;
                    break;
                }
            }
            // 如果当前BAR.HighestPrice为0，则用当前最新价赋值，否则判断是否小于最新价，小于则替换
            if ((m_array1mKLine[t_iInstrumentIndex] + t_i1MinOffset)->HighestPrice == 0)
            {
                (m_array1mKLine[t_iInstrumentIndex] + t_i1MinOffset)->HighestPrice = m_hashMarketDataList[pInstrument].LastPrice;
            }
            else if ((m_array1mKLine[t_iInstrumentIndex] + t_i1MinOffset)->HighestPrice < m_hashMarketDataList[pInstrument].LastPrice)
            {
                (m_array1mKLine[t_iInstrumentIndex] + t_i1MinOffset)->HighestPrice = m_hashMarketDataList[pInstrument].LastPrice;
            }
            // 如果当前BAR.LowestPrice为0，则用当前最新价赋值，否则判断是否大于最新价，大于则替换
            if ((m_array1mKLine[t_iInstrumentIndex] + t_i1MinOffset)->LowestPrice == 0)
            {
                (m_array1mKLine[t_iInstrumentIndex] + t_i1MinOffset)->LowestPrice = m_hashMarketDataList[pInstrument].LastPrice;
            }
            else if ((m_array1mKLine[t_iInstrumentIndex] + t_i1MinOffset)->LowestPrice > m_hashMarketDataList[pInstrument].LastPrice)
            {
                (m_array1mKLine[t_iInstrumentIndex] + t_i1MinOffset)->LowestPrice = m_hashMarketDataList[pInstrument].LastPrice;
            }
            // 持续更新收盘价为当前价
            (m_array1mKLine[t_iInstrumentIndex] + t_i1MinOffset)->ClosePrice = m_hashMarketDataList[pInstrument].LastPrice;
            // 当前K线成交量=当前行情内累积成交量-BAR[0]中保存的上条K线累积成交量
            (m_array1mKLine[t_iInstrumentIndex] + t_i1MinOffset)->Volume = m_hashMarketDataList[pInstrument].Volume - (m_array1mKLine[t_iInstrumentIndex] + 0)->Volume;
            // 当前K线成交额=当前行情内累积成交额-BAR[0]中保存的上条K线累积成交额
            (m_array1mKLine[t_iInstrumentIndex] + t_i1MinOffset)->Turnover = m_hashMarketDataList[pInstrument].Turnover - (m_array1mKLine[t_iInstrumentIndex] + 0)->Turnover;
            /*std::cout << "  " << t_iInstrumentIndex << "," << 0 << ":" << (m_array1mKLine[t_iInstrumentIndex]+0)->TradingDay << ','
            << (m_array1mKLine[t_iInstrumentIndex]+0)->Contract << ','
            << (m_array1mKLine[t_iInstrumentIndex]+0)->SecondsPeriod << ','
            << (m_array1mKLine[t_iInstrumentIndex]+0)->BarSerials << ','
            << (m_array1mKLine[t_iInstrumentIndex]+0)->begintime << ','
            << (m_array1mKLine[t_iInstrumentIndex]+0)->endtime << ','
            << (m_array1mKLine[t_iInstrumentIndex]+0)->OpenPrice << ','
            << (m_array1mKLine[t_iInstrumentIndex]+0)->HighestPrice << ','
            << (m_array1mKLine[t_iInstrumentIndex]+0)->LowestPrice << ','
            << (m_array1mKLine[t_iInstrumentIndex]+0)->ClosePrice << ','
            << (m_array1mKLine[t_iInstrumentIndex]+0)->Volume << ','
            << (m_array1mKLine[t_iInstrumentIndex]+0)->Turnover
            << std::endl;
            std::cout << "  " << t_iInstrumentIndex << "," << t_i1MinOffset << ":" << (m_array1mKLine[t_iInstrumentIndex]+t_i1MinOffset)->TradingDay << ','
            << (m_array1mKLine[t_iInstrumentIndex]+t_i1MinOffset)->Contract << ','
            << (m_array1mKLine[t_iInstrumentIndex]+t_i1MinOffset)->SecondsPeriod << ','
            << (m_array1mKLine[t_iInstrumentIndex]+t_i1MinOffset)->BarSerials << ','
            << (m_array1mKLine[t_iInstrumentIndex]+t_i1MinOffset)->begintime << ','
            << (m_array1mKLine[t_iInstrumentIndex]+t_i1MinOffset)->endtime << ','
            << (m_array1mKLine[t_iInstrumentIndex]+t_i1MinOffset)->OpenPrice << ','
            << (m_array1mKLine[t_iInstrumentIndex]+t_i1MinOffset)->HighestPrice << ','
            << (m_array1mKLine[t_iInstrumentIndex]+t_i1MinOffset)->LowestPrice << ','
            << (m_array1mKLine[t_iInstrumentIndex]+t_i1MinOffset)->ClosePrice << ','
            << (m_array1mKLine[t_iInstrumentIndex]+t_i1MinOffset)->Volume << ','
            << (m_array1mKLine[t_iInstrumentIndex]+t_i1MinOffset)->Turnover
            << std::endl;*/
#pragma endregion KBar1m
            /*
            * 获得3分钟K线偏移量，记录K线数据
            * 偏移量从1开始 0记录当前行情
            */
#pragma region KBar3m
            int t_i3MinOffset = m_TradingTimeLine[t_iTimeLineTypeIndex][t_iTimeLineindex].BarSerials_3Min;
            /*
            * 保存行情累积值,K线切换时计算,切换标志为当前BAR.closeprice为0,切换后closeprice被lastprice刷新
            */
            if ((m_array3mKLine[t_iInstrumentIndex] + t_i3MinOffset)->ClosePrice == 0)
            {
                // 高开低收为上条BAR的值
                (m_array3mKLine[t_iInstrumentIndex] + 0)->OpenPrice = (m_array3mKLine[t_iInstrumentIndex] + t_i3MinOffset - 1)->OpenPrice;
                (m_array3mKLine[t_iInstrumentIndex] + 0)->HighestPrice = (m_array3mKLine[t_iInstrumentIndex] + t_i3MinOffset - 1)->HighestPrice;
                (m_array3mKLine[t_iInstrumentIndex] + 0)->LowestPrice = (m_array3mKLine[t_iInstrumentIndex] + t_i3MinOffset - 1)->LowestPrice;
                (m_array3mKLine[t_iInstrumentIndex] + 0)->ClosePrice = (m_array3mKLine[t_iInstrumentIndex] + t_i3MinOffset - 1)->ClosePrice;
                // 累积之前所有BAR时的成交量
                (m_array3mKLine[t_iInstrumentIndex] + 0)->Volume += (m_array3mKLine[t_iInstrumentIndex] + t_i3MinOffset - 1)->Volume;
                // 累积之前所有BAR时的成交量
                (m_array3mKLine[t_iInstrumentIndex] + 0)->Turnover += (m_array3mKLine[t_iInstrumentIndex] + t_i3MinOffset - 1)->Turnover;
            }
            /*
            * 保存对应K线BAR
            * 当天第一条无法取上一条BAR的值，需要特殊处理
            */
            // 如果当前BAR.openprice为0，则需要赋值，用上一条BAR.closeprice或者用当天的开盘价
            if ((m_array3mKLine[t_iInstrumentIndex] + t_i3MinOffset)->OpenPrice == 0)
            {
                switch (t_i3MinOffset)
                {
                    // 如果为第一根BAR，则openprice为当天行情开盘价，该数据在当前行情数据中
                case 1:(m_array3mKLine[t_iInstrumentIndex] + t_i3MinOffset)->OpenPrice = m_hashMarketDataList[pInstrument].OpenPrice;
                    break;
                    /*
                    * 如果为其他BAR，则用上一条BAR的收盘价作为开盘价
                    * 但如果BAR记录不全，即前面缺失，则用当天开盘价作为openprice
                    */
                default:(m_array3mKLine[t_iInstrumentIndex] + t_i3MinOffset)->OpenPrice
                    = (m_array3mKLine[t_iInstrumentIndex] + t_i3MinOffset - 1)->ClosePrice == 0 ? m_hashMarketDataList[pInstrument].OpenPrice : (m_array3mKLine[t_iInstrumentIndex] + t_i3MinOffset - 1)->ClosePrice;
                    break;
                }
            }
            // 如果当前BAR.HighestPrice为0，则用当前最新价赋值，否则判断是否小于最新价，小于则替换
            if ((m_array3mKLine[t_iInstrumentIndex] + t_i3MinOffset)->HighestPrice == 0)
            {
                (m_array3mKLine[t_iInstrumentIndex] + t_i3MinOffset)->HighestPrice = m_hashMarketDataList[pInstrument].LastPrice;
            }
            else if ((m_array3mKLine[t_iInstrumentIndex] + t_i3MinOffset)->HighestPrice < m_hashMarketDataList[pInstrument].LastPrice)
            {
                (m_array3mKLine[t_iInstrumentIndex] + t_i3MinOffset)->HighestPrice = m_hashMarketDataList[pInstrument].LastPrice;
            }
            // 如果当前BAR.LowestPrice为0，则用当前最新价赋值，否则判断是否大于最新价，大于则替换
            if ((m_array3mKLine[t_iInstrumentIndex] + t_i3MinOffset)->LowestPrice == 0)
            {
                (m_array3mKLine[t_iInstrumentIndex] + t_i3MinOffset)->LowestPrice = m_hashMarketDataList[pInstrument].LastPrice;
            }
            else if ((m_array3mKLine[t_iInstrumentIndex] + t_i3MinOffset)->LowestPrice > m_hashMarketDataList[pInstrument].LastPrice)
            {
                (m_array3mKLine[t_iInstrumentIndex] + t_i3MinOffset)->LowestPrice = m_hashMarketDataList[pInstrument].LastPrice;
            }
            // 持续更新收盘价为当前价
            (m_array3mKLine[t_iInstrumentIndex] + t_i3MinOffset)->ClosePrice = m_hashMarketDataList[pInstrument].LastPrice;
            // 当前K线成交量=当前行情内累积成交量-BAR[0]中保存的上条K线累积成交量
            (m_array3mKLine[t_iInstrumentIndex] + t_i3MinOffset)->Volume = m_hashMarketDataList[pInstrument].Volume - (m_array3mKLine[t_iInstrumentIndex] + 0)->Volume;
            // 当前K线成交额=当前行情内累积成交额-BAR[0]中保存的上条K线累积成交额
            (m_array3mKLine[t_iInstrumentIndex] + t_i3MinOffset)->Turnover = m_hashMarketDataList[pInstrument].Turnover - (m_array3mKLine[t_iInstrumentIndex] + 0)->Turnover;
#pragma endregion KBar3m
            /*
            * 获得5分钟K线偏移量，记录K线数据
            * 偏移量从1开始 0记录当前行情
            */
#pragma region KBar5m
            int t_i5MinOffset = m_TradingTimeLine[t_iTimeLineTypeIndex][t_iTimeLineindex].BarSerials_5Min;
#pragma endregion KBar5m
            /*
            * 获得10分钟K线偏移量，记录K线数据
            * 偏移量从1开始 0记录当前行情
            */
#pragma region KBar10m
            int t_i10MinOffset = m_TradingTimeLine[t_iTimeLineTypeIndex][t_iTimeLineindex].BarSerials_10Min;
#pragma endregion KBar10m
            /*
            * 获得15分钟K线偏移量，记录K线数据
            * 偏移量从1开始 0记录当前行情
            */
#pragma region KBar15m
            int t_i15MinOffset = m_TradingTimeLine[t_iTimeLineTypeIndex][t_iTimeLineindex].BarSerials_15Min;
#pragma endregion KBar15m
            /*
            * 获得30分钟K线偏移量，记录K线数据
            * 偏移量从1开始 0记录当前行情
            */
#pragma region KBar30m
            int t_i30MinOffset = m_TradingTimeLine[t_iTimeLineTypeIndex][t_iTimeLineindex].BarSerials_30Min;
#pragma endregion KBar30m
            /*
            * 获得60分钟K线偏移量，记录K线数据
            * 偏移量从1开始 0记录当前行情
            */
#pragma region KBar60m
            int t_i60MinOffset = m_TradingTimeLine[t_iTimeLineTypeIndex][t_iTimeLineindex].BarSerials_60Min;
#pragma endregion KBar60m
        }
        else
        {
            sprintf_s(t_strLog, "%s:%s%s%s", t_strLogFuncName, "未找到", pInstrument, "的K线时间线");
            LOG4CPLUS_ERROR(m_objLogger, t_strLog);
            return;
        }
    }
    else
    {
        sprintf_s(t_strLog, "%s:%s%s%s", t_strLogFuncName, "未找到", pInstrument, "K线存储位置");
        LOG4CPLUS_ERROR(m_objLogger, t_strLog);
    }
}
#endif KLINESTORAGE

/// 订阅多个行情
void axapi::MarketQuotationAPI::subMarketData(char *pInstrumentList[], int in_nInstrumentCount)
{
    char *t_strLogFuncName = "axapi::MarketQuotationAPI::subMarketData";
    char t_strLog[500];
    sprintf_s(t_strLog, "%s:%s%d%s", t_strLogFuncName, "开始订阅", in_nInstrumentCount, "个合约的行情");
    LOG4CPLUS_INFO(m_objLogger, t_strLog);

    while (true)
    {
        if (m_pUserApi->SubscribeMarketData(pInstrumentList, in_nInstrumentCount) == 0)
        {
            sprintf_s(t_strLog, "%s:%s", t_strLogFuncName, "行情订阅成功");
            LOG4CPLUS_INFO(m_objLogger, t_strLog);
            break;
        }
    }
}

/// 订阅一个行情，初始化K线存储空间
int axapi::MarketQuotationAPI::subMarketDataSingle(char *in_strInstrument)
{
    char *t_strLogFuncName = "MarketDataAPI_CTP::subMarketDataSingle";
    char t_strLog[500];
    sprintf_s(t_strLog, "%s:%s%s%s", t_strLogFuncName, "开始订阅合约", in_strInstrument, "的行情");
    LOG4CPLUS_INFO(m_objLogger, t_strLog);

    if (m_hashMarketDataList.find(in_strInstrument) == m_hashMarketDataList.end())
    {
        struct MarketDataField t_MarketDataField;
        memset(&t_MarketDataField, '\0', sizeof(MarketDataField));
        strcpy_s(t_MarketDataField.InstrumentID, sizeof(t_MarketDataField.InstrumentID), in_strInstrument);
        m_hashMarketDataList[in_strInstrument] = t_MarketDataField;
        sprintf_s(t_strLog, "%s:%s%zd", t_strLogFuncName, "m_hashMarketDataList当前含有用户数:", m_hashMarketDataList.size());
        LOG4CPLUS_DEBUG(m_objLogger, t_strLog);

        /*hash_map<string, struct MarketDataField>::iterator t_iterator;
        for(t_iterator=m_hashMarketDataList.begin();t_iterator!=m_hashMarketDataList.end();t_iterator++)
        {
        cout << "key:" << t_iterator->first << "|" << t_iterator->second.InstrumentID << "," << t_iterator->second.LastPrice << endl;
        }*/

        char **t_InstrumentList = new char*[1];
        t_InstrumentList[0] = in_strInstrument;

        if (m_pUserApi->SubscribeMarketData(t_InstrumentList, 1) == 0)
        {
            sprintf_s(t_strLog, "%s:%s", t_strLogFuncName, "行情订阅成功");
            LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
#ifdef KLINESTORAGE
            /// 初始化K线
            initialKMarketDataSingle(in_strInstrument);
#endif KLINESTORAGE
            return 0;
        }
        else
        {
            return -100;
        }
    }
    else
    {
        return 0;
    }
}

/// 得到指定合约的行情
double axapi::MarketQuotationAPI::getCurrPrice(const char *in_strInstrument)
{
    char *t_strLogFuncName = "axapi::MarketQuotationAPI::getCurrPrice";
    char t_strLog[500];

    sprintf_s(t_strLog, "%s:%s%zd", t_strLogFuncName, "m_hashMarketDataList当前含有用户数:", m_hashMarketDataList.size());
    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);

    if (m_hashMarketDataList.find(in_strInstrument) != m_hashMarketDataList.end())
    {
        sprintf_s(t_strLog, "%s:%s", t_strLogFuncName, "找到请求行情合约");
        LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
        return m_hashMarketDataList[in_strInstrument].LastPrice;
    }
    return -100;
}

/// 得到指定合约的行情
axapi::MarketDataField *axapi::MarketQuotationAPI::getCurrentPrice(const char *in_strInstrument)
{
    char *t_strLogFuncName = "axapi::MarketQuotationAPI::getCurrentPrice";
    char t_strLog[500];

    sprintf_s(t_strLog, "%s:%s%zd", t_strLogFuncName, "m_hashMarketDataList当前含有用户数:", m_hashMarketDataList.size());
    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);

    if (m_hashMarketDataList.find(in_strInstrument) != m_hashMarketDataList.end())
    {
        sprintf_s(t_strLog, "%s:%s", t_strLogFuncName, "找到请求行情合约");
        LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
        return &m_hashMarketDataList[in_strInstrument];
    }
    return NULL;
}

#ifdef KLINESTORAGE
///得到指定合约的K线数据, 指定与最新K线的偏移量, 默认得到最新的1分钟K线数据
axapi::KMarketDataField *axapi::MarketQuotationAPI::getKLine(const char *in_strInstrument, int in_iSecondsPeriod, int in_iCurrentOffset)
{
    /// 获得品种对应时间轴,不同品种对应不同时间轴
    int t_iTimeLineTypeIndex = 0;
    /// 获得所要K线的时间轴偏移量
    int t_iTimeLineindex = findCurrentTradingTimeLineOffset(in_strInstrument);
    /// 获得当前合约存储位置索引
    int t_iInstrumentIndex = findMarketDataIndex(in_strInstrument);
    if (t_iInstrumentIndex < 0 || t_iTimeLineindex <= 0)
    {
        return NULL;
    }

    int t_iNMinOffset;
    switch (in_iSecondsPeriod)
    {
    case 1:  t_iNMinOffset = m_TradingTimeLine[t_iTimeLineTypeIndex][t_iTimeLineindex].BarSerials_1Min; break;
    case 3:  t_iNMinOffset = m_TradingTimeLine[t_iTimeLineTypeIndex][t_iTimeLineindex].BarSerials_3Min; break;
    case 5:  t_iNMinOffset = m_TradingTimeLine[t_iTimeLineTypeIndex][t_iTimeLineindex].BarSerials_5Min; break;
    case 10: t_iNMinOffset = m_TradingTimeLine[t_iTimeLineTypeIndex][t_iTimeLineindex].BarSerials_10Min; break;
    case 15: t_iNMinOffset = m_TradingTimeLine[t_iTimeLineTypeIndex][t_iTimeLineindex].BarSerials_15Min; break;
    case 30: t_iNMinOffset = m_TradingTimeLine[t_iTimeLineTypeIndex][t_iTimeLineindex].BarSerials_30Min; break;
    case 60: t_iNMinOffset = m_TradingTimeLine[t_iTimeLineTypeIndex][t_iTimeLineindex].BarSerials_60Min; break;
    default: return NULL;
    }
    //std::cout << "t_iNMinOffset:" << t_iNMinOffset<< std::endl;
    if (t_iNMinOffset + in_iCurrentOffset > 0)
    {
        return m_array1mKLine[t_iInstrumentIndex] + t_iNMinOffset + in_iCurrentOffset;
    }
    else
    {
        return NULL;
    }
}
#endif KLINESTORAGE

#ifdef KLINESTORAGE
/// 得到指定合约的K线数据,指定所处当天K线的位置,默认得到第一分钟的1分钟K线数据
axapi::KMarketDataField *axapi::MarketQuotationAPI::getKLineBySerials(const char *in_strInstrument, int in_iSecondsPeriod, int in_iPosition)
{
    /// 获得品种对应时间轴,不同品种对应不同时间轴
    int t_iTimeLineTypeIndex = 0;
    /// 获得所要K线的最大BARS数量
    int t_iTimeLineMaxIndex = 0;
    switch (in_iSecondsPeriod)
    {
    case 1:  t_iTimeLineMaxIndex = BARNUMBER_1MIN; break;
    case 3:  t_iTimeLineMaxIndex = BARNUMBER_3MIN; break;
    case 5:  t_iTimeLineMaxIndex = BARNUMBER_5MIN; break;
    case 10: t_iTimeLineMaxIndex = BARNUMBER_10MIN; break;
    case 15: t_iTimeLineMaxIndex = BARNUMBER_15MIN; break;
    case 30: t_iTimeLineMaxIndex = BARNUMBER_30MIN; break;
    case 60: t_iTimeLineMaxIndex = BARNUMBER_60MIN; break;
    default: return NULL;
    }
    /// 获得当前合约存储位置索引
    int t_iInstrumentIndex = findMarketDataIndex(in_strInstrument);

    if (t_iInstrumentIndex < 0 || t_iTimeLineMaxIndex <= in_iPosition || in_iPosition <= 0)
    {
        return NULL;
    }

    //std::cout << "t_iNMinOffset:" << t_iNMinOffset<< std::endl;
    return m_array1mKLine[t_iInstrumentIndex] + in_iPosition;
}
#endif KLINESTORAGE

/// 获得接口运行状态
axapi::APIStatus axapi::MarketQuotationAPI::getAPIStatus()
{
    return m_nAPIStatus;
}

/// 设置接口状态
void axapi::MarketQuotationAPI::setAPIStatus(APIStatus in_nAPIStatus)
{
    char *t_strLogFuncName = "axapi::MarketQuotationAPI::setAPIStatus";
    char t_strLog[500];

    sprintf_s(t_strLog, "%s:%d", t_strLogFuncName, in_nAPIStatus);
    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);

    m_nAPIStatus = in_nAPIStatus;
}

/// 判断传入时间是否大于指定交易所时间
bool axapi::MarketQuotationAPI::overTime(char *in_strExchName, tm *in_ptime)
{
    if (getExchangeTime(in_strExchName) > (in_ptime->tm_hour * 3600 + in_ptime->tm_min * 60 + in_ptime->tm_sec))
    {
        return true;
    }
    else
    {
        return false;
    }
}

/// 获取本地时间
double axapi::MarketQuotationAPI::getLocalTime()
{
    time_t nowtime = time(NULL);
    tm *strnowtime;
    strnowtime = localtime(&nowtime); //获取当前时间
    return (strnowtime->tm_hour) * 60 * 60 + (strnowtime->tm_min) * 60 + (strnowtime->tm_sec);
}

/// 通过交易所名字获取到交易所时间（本地时间+/-交易所时间差）
double axapi::MarketQuotationAPI::getExchangeTime(char *in_strExchName)
{
    if (strcmp(in_strExchName, "CFFEX") == 0)
    {
        return getLocalTime() + m_nCFFEXTimeDiff;
    }
    else if (strcmp(in_strExchName, "DCE") == 0)
    {
        return getLocalTime() + m_nDCETimeDiff;
    }
    else if (strcmp(in_strExchName, "CZCE") == 0)
    {
        return getLocalTime() + m_nCZCETimeDiff;
    }
    else if (strcmp(in_strExchName, "SHFE") == 0)
    {
        return getLocalTime() + m_nSHFETimeDiff;
    }
    else if (strcmp(in_strExchName, "IME") == 0)
    {
        return getLocalTime() + m_nIMETimeDiff;
    }
    return getLocalTime();
}

/// 通过时间格式字符串(hh24:mi:ss)获取时间
double axapi::MarketQuotationAPI::getTimebyFormat(char *in_strtime)
{
    int t_strlength = 0;
    int t_delimscount = 0;
    int t_multiply = 3600;
    char t_ntime[3];
    double t_dbtime = 0;
    try
    {
        memset(t_ntime, '\0', sizeof(t_ntime));

        while (true)
        {
            if (in_strtime[t_strlength] < '0' || in_strtime[t_strlength] > '9')
            {
                return -200;
            }
            else
            {
                for (int i = 0; i < 3; i++)
                {
                    if (t_ntime[i] == '\0')
                    {
                        t_ntime[i] = in_strtime[t_strlength];
                        break;
                    }
                }
            }
            if (in_strtime[t_strlength] == ':')
            {
                t_delimscount++;
                t_dbtime += atoi(t_ntime)*t_multiply;
                t_multiply /= 60;
                memset(t_ntime, '\0', sizeof(t_ntime));
            }
            if (in_strtime[t_strlength] == '\0')
            {
                t_strlength++;
                if (t_strlength > 9)
                {
                    return -300;
                }
                t_dbtime += atoi(t_ntime)*t_multiply;
                break;
            }
            t_strlength++;
        }
        if (t_delimscount != 2)
            return -100;
    }
    catch (std::exception e)
    {
        return -400;
    }
    return t_dbtime;
}

#ifdef HQDATAFILE
/// 初始化实时行情写文件组件
void axapi::MarketQuotationAPI::initializeHQDataIntoFiles()
{
    char *t_strLogFuncName = "axapi::MarketQuotationAPI::initializeHQDataIntoFiles";
    char t_strLog[500];

    m_pHQDataIntoFiles = new DataIntoFiles("MarketQuotationHQDATAFILE", 50000, "MarketQuotationHQDATAFILEs.list");
    sprintf_s(t_strLog, "%s:%s", t_strLogFuncName, "初始化实时行情写文件组件完成");
    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
}
#endif HQDATAFILE

/// 实时行情落地
void axapi::MarketQuotationAPI::writeHQData(APINamespace CThostFtdcDepthMarketDataField *pDepthMarketData)
{
    char *t_strLogFuncName = "axapi::MarketQuotationAPI::writeHQData";
    char t_strLog[500];
    try {
        char t_strMarketData[2000];
        sprintf_s(t_strMarketData, 1999, "%s,%s,%s,%s,\
                                   %s,%s,%d,\
                                   %lf,%lf,%lf,%lf,\
                                   %lf,%d,%lf,\
                                   %lf,%lf,\
                                   %lf,%lf,%lf,%lf,\
                                   %lf,%lf,%lf,%lf,\
                                   %lf,%d,%lf,%d,\
                                   %lf,%d,%lf,%d,\
                                   %lf,%d,%lf,%d,\
                                   %lf,%d,%lf,%d,\
                                   %lf,%d,%lf,%d,\
                                   ",
            pDepthMarketData->TradingDay, pDepthMarketData->ExchangeID, pDepthMarketData->ExchangeInstID, pDepthMarketData->InstrumentID,
            pDepthMarketData->ActionDay, pDepthMarketData->UpdateTime, pDepthMarketData->UpdateMillisec,
            pDepthMarketData->OpenPrice, pDepthMarketData->HighestPrice, pDepthMarketData->LowestPrice, pDepthMarketData->ClosePrice,
            pDepthMarketData->LastPrice, pDepthMarketData->Volume, pDepthMarketData->Turnover,
            pDepthMarketData->UpperLimitPrice, pDepthMarketData->LowerLimitPrice,
            pDepthMarketData->AveragePrice, pDepthMarketData->SettlementPrice, pDepthMarketData->CurrDelta, pDepthMarketData->OpenInterest,
            pDepthMarketData->PreSettlementPrice, pDepthMarketData->PreClosePrice, pDepthMarketData->PreOpenInterest, pDepthMarketData->PreDelta,
            pDepthMarketData->BidPrice1, pDepthMarketData->BidVolume1, pDepthMarketData->AskPrice1, pDepthMarketData->AskVolume1,
            pDepthMarketData->BidPrice2, pDepthMarketData->BidVolume2, pDepthMarketData->AskPrice2, pDepthMarketData->AskVolume2,
            pDepthMarketData->BidPrice3, pDepthMarketData->BidVolume3, pDepthMarketData->AskPrice3, pDepthMarketData->AskVolume3,
            pDepthMarketData->BidPrice4, pDepthMarketData->BidVolume4, pDepthMarketData->AskPrice4, pDepthMarketData->AskVolume4,
            pDepthMarketData->BidPrice5, pDepthMarketData->BidVolume5, pDepthMarketData->AskPrice5, pDepthMarketData->AskVolume5
        );
#ifdef HQDATAFILE
        if (m_pHQDataIntoFiles != NULL)
        {
            m_pHQDataIntoFiles->writeData2File(t_strMarketData);
            sprintf_s(t_strLog, "%s:%s", t_strLogFuncName, "实时行情写入文件正常");
            LOG4CPLUS_TRACE(m_objLogger, t_strLog);
        }
        else
        {
            sprintf_s(t_strLog, "%s:%s", t_strLogFuncName, "实时行情无法写入文件，文件组件异常");
            LOG4CPLUS_WARN(m_objLogger, t_strLog);
        }
#endif HQDATAFILE
    }
    catch (std::exception e)
    {
        sprintf_s(t_strLog, "%s:%s", t_strLogFuncName, e.what());
        LOG4CPLUS_WARN(m_objLogger, t_strLog);
        }
    }

#ifdef KDATAFILE
/// 初始化实时行情写文件组件
void axapi::MarketQuotationAPI::initializeKBarDataIntoFiles()
{
    char *t_strLogFuncName = "axapi::MarketQuotationAPI::initializeKBarDataIntoFiles";
    char t_strLog[500];

    m_pKBarDataIntoFiles = new DataIntoFiles("MarketQuotationKDATAFILE", 50000, "MarketQuotationKDATAFILEs.list");
    sprintf_s(t_strLog, "%s:%s", t_strLogFuncName, "初始化KBar写文件组件完成");
    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
}
#endif KDATAFILE

#ifdef KLINESTORAGE
/// KBar落地
void axapi::MarketQuotationAPI::writeKBarData()
{
    char *t_strLogFuncName = "axapi::MarketQuotationAPI::writeKBarData";
    char t_strLog[500];
    try
    {
        int t_iTimeLineTypeIndex = 0;
        for (unsigned int i = 0; i < m_arrayContracts.size(); i++)
        {
            int t_iTimeLineindex = findCurrentTradingTimeLineOffset(m_arrayContracts[i].InstrumentID);
            KMarketDataField *t_pKMarketDataField = m_array1mKLine[i] + m_TradingTimeLine[t_iTimeLineTypeIndex][t_iTimeLineindex].BarSerials_1Min;
            char t_strKMarketData[500];
            sprintf_s(t_strKMarketData, "%s,%s,%d,\
                                        %d,%ld,%ld,\
                                        %lf,%lf,%lf,%lf,\
                                        %lf,%lf",
                t_pKMarketDataField->TradingDay, t_pKMarketDataField->Contract, t_pKMarketDataField->SecondsPeriod,
                t_pKMarketDataField->BarSerials, t_pKMarketDataField->begintime, t_pKMarketDataField->endtime,
                t_pKMarketDataField->OpenPrice, t_pKMarketDataField->HighestPrice, t_pKMarketDataField->LowestPrice, t_pKMarketDataField->ClosePrice,
                t_pKMarketDataField->Turnover, t_pKMarketDataField->Volume
            );
#ifdef KDATAFILE
            if (m_pKBarDataIntoFiles != NULL)
            {
                m_pKBarDataIntoFiles->writeData2File(t_strKMarketData);
                sprintf_s(t_strLog, "%s:%s", t_strLogFuncName, "KBar写入文件正常");
                LOG4CPLUS_TRACE(m_objLogger, t_strLog);
            }
            else
            {
                sprintf_s(t_strLog, "%s:%s", t_strLogFuncName, "KBar无法写入文件，文件组件异常");
                LOG4CPLUS_WARN(m_objLogger, t_strLog);
            }
#endif KDATAFILE
        }
    }
    catch (std::exception e)
    {
        sprintf_s(t_strLog, "%s:%s", t_strLogFuncName, e.what());
        LOG4CPLUS_WARN(m_objLogger, t_strLog);
            }
        }
#endif KLINESTORAGE