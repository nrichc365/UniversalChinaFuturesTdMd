/*
* modified by niuchao
* on Mar. 11th, 2019
* 1.上穿或者下穿均线MA下单，N秒平仓，止损平仓
*/
#define TRADEAPI_VERSION
//#define CTP_TRADEAPI
#define KSV6T_TRADEAPI
#define MEMORYDATA
#define KLINESTORAGE
#define _CRT_SECURE_NO_WARNINGS
#include "TradeAPI.h"
#include "TradeAPITypeDefine.h"
#include "MarketQuotationAPI.h"
#include <iostream>
#include "configRead_dll/readConfig.h"
#include <thread>
#include <mutex>
#include <time.h>

/// 用于日志
log4cplus::Logger g_root;
log4cplus::Logger g_objLogger_DEBUG, g_objLogger_INFO;
char g_strLog[500];

// 用于清仓
std::mutex gmutex;
bool g_blOffsetALLFlag = false;
bool g_blOffsetFlag = true;
bool g_blOpenFlag = true;
bool g_blCancelFlag = true;
bool g_blShutdownFlag = false;

// 撤单等待间隔
int g_nCancelWaitSeconds = 0;

/// TODO:策略变量定义
#pragma region
int g_nProfitPoint2Offset;
#pragma endregion

//void InstrumentStatus(axapi::TradeAPI* t_tradeapi, char *t_strInstrument)
//{
//    while(true)
//    {
//        if(t_tradeapi->getInstrumentStatusInfo(t_strInstrument).InstrumentStatus != '\0')
//        {
//            break;
//            for(unsigned int i=0; i<=t_tradeapi->sizeInstrumentStatusList(); i++)
//            {
//                strcmp(t_tradeapi->getInstrumentStatusInfo(i).InstrumentID, 
//            }
//        }
//    }
//}

/// 时钟监控,设置各开关
void TradeTimeMonitor(int t_nOffsetAllTime)
{
    while (true)
    {
        time_t nowtime = time(NULL);
        tm *curtime = localtime(&nowtime);
        //std::cout << "current time:" << curtime->tm_hour << ":" << curtime->tm_min << std::endl;
        //std::cout << "false:" << false << "|" << (curtime->tm_hour >= 14 && curtime->tm_min >= t_nOffsetAllTime) << endl;
        //std::cout << "false:" << false << "|" << ((curtime->tm_hour >= 14 && curtime->tm_min >= t_nOffsetAllTime) || curtime->tm_hour >= 15) << endl;

        /// 14:??-19:59为清盘时间
        if (((curtime->tm_hour >= 14 && curtime->tm_min >= t_nOffsetAllTime) || curtime->tm_hour >= 15) && curtime->tm_hour < 20)
        {
            sprintf_s(g_strLog, "到清盘时间:%d:%d:%d", curtime->tm_hour, curtime->tm_min, curtime->tm_sec);
            LOG4CPLUS_DEBUG(g_objLogger_DEBUG, g_strLog);
            /// 停止开仓
            if (g_blOpenFlag)
                g_blOpenFlag = false;
            /// 停止止损
            if (g_blOffsetFlag)
                g_blOffsetFlag = false;
        }
        Sleep(1000);
    }
}

//跟单线程 目前跟单由主线程处理
void Order()
{
    try
    {
        gmutex.lock();
        gmutex.unlock();
    }
    catch (std::exception e)
    {
        sprintf_s(g_strLog, "Order:%s", e.what());
        LOG4CPLUS_DEBUG(g_objLogger_DEBUG, g_strLog);
    }
}

//撤单线程
#ifdef TRADEAPI_VERSION
void CancelOrder(axapi::TradeAPI* t_tradeapi, axapi::MarketQuotationAPI* t_marketapi)
#endif TRADEAPI_VERSION
{
    try {
        //用于获取时间
        time_t nowtime;
        tm *curtime;
        int curMinutes, t_OrderInsertTime;
        char t_TradeInfoHour[3];
        char t_TradeInfoMinutes[3];
        char t_TradeInfoSeconds[3];
        //是否撤单标志
        bool t_CancelAction;

        while (g_blCancelFlag)
        {
            int t_ordersize = t_tradeapi->sizeOrderList();
            sprintf_s(g_strLog, "CancelOrder:sizeOrderList=%d", t_ordersize);
            LOG4CPLUS_DEBUG(g_objLogger_DEBUG, g_strLog);
            for (int i = 1; i <= t_ordersize&&g_blCancelFlag; i++)
            {
                APINamespace CThostFtdcOrderField t_objOrderInfo = t_tradeapi->getOrderInfo(i);
                //std::cout << "CancelOrder:" << i << ":" << t_objOrderInfo.InstrumentID << "|" << t_objOrderInfo.OrderSysID << "|" << t_objOrderInfo.OrderStatus << std::endl;
                //Sleep(100);
                if (t_objOrderInfo.OrderSysID[0] == '\0')
                {
                    //超出范围
                    continue;
                }
                if (strcmp(t_objOrderInfo.OrderSysID, "0") == 0)
                {
                    //错单不做撤单处理
                    //std::cout << i << ":错单不处理" << std::endl;
                    continue;
                }
                //std::cout << i << ":" << t_objOrderInfo.OrderSysID << '|';

                //检索委托，找到需要撤单的委托
                switch (t_objOrderInfo.OrderStatus)
                {
                    //需要撤单
                case THOST_FTDC_OST_PartTradedQueueing:
                case THOST_FTDC_OST_NoTradeQueueing:
                {
                    /*
                    * 是否撤单
                    * 1.n秒撤单
                    */
#pragma region
                    nowtime = time(NULL);
                    curtime = localtime(&nowtime);
                    t_TradeInfoHour[0] = t_objOrderInfo.InsertTime[0];
                    t_TradeInfoHour[1] = t_objOrderInfo.InsertTime[1];
                    t_TradeInfoHour[2] = '\0';
                    t_TradeInfoMinutes[0] = t_objOrderInfo.InsertTime[3];
                    t_TradeInfoMinutes[1] = t_objOrderInfo.InsertTime[4];
                    t_TradeInfoMinutes[2] = '\0';
                    t_TradeInfoSeconds[0] = t_objOrderInfo.InsertTime[6];
                    t_TradeInfoSeconds[1] = t_objOrderInfo.InsertTime[7];
                    t_TradeInfoSeconds[2] = '\0';
                    if (curtime->tm_hour < 20)
                    {
                        curMinutes = (curtime->tm_hour) * 3600 + curtime->tm_min * 60 + curtime->tm_sec * 1;
                        t_OrderInsertTime = atoi(t_TradeInfoHour) * 3600 + atoi(t_TradeInfoMinutes) * 60 + atoi(t_TradeInfoSeconds);
                    }
                    else
                    {
                        curMinutes = (curtime->tm_hour + 0) * 3600 + curtime->tm_min * 60 + curtime->tm_sec * 1;
                        t_OrderInsertTime = atoi(t_TradeInfoHour) * 3600 + atoi(t_TradeInfoMinutes) * 60 + atoi(t_TradeInfoSeconds);
                    }
                    sprintf_s(g_strLog, "curMinutes:%d,|%d:%d:%d", curMinutes, curtime->tm_hour, curtime->tm_min, curtime->tm_sec);
                    LOG4CPLUS_DEBUG(g_objLogger_DEBUG, g_strLog);
                    sprintf_s(g_strLog, "t_OrderInfoHour:%d,%s|%s|%s|%s:%s:%s", t_OrderInsertTime, t_objOrderInfo.OrderSysID, t_objOrderInfo.OrderRef, t_objOrderInfo.InsertTime, t_TradeInfoHour, t_TradeInfoMinutes, t_TradeInfoSeconds);
                    LOG4CPLUS_DEBUG(g_objLogger_DEBUG, g_strLog);
                    sprintf_s(g_strLog, "curMinutes:%d,t_OrderInsertTime:%d", curMinutes, t_OrderInsertTime);
                    LOG4CPLUS_DEBUG(g_objLogger_DEBUG, g_strLog);

                    if (curMinutes - t_OrderInsertTime >= g_nCancelWaitSeconds)
                    {
                        t_CancelAction = true;
                    }
                    else
                    {
                        t_CancelAction = false;
                    }
#pragma endregion
                    if (t_CancelAction)
                    {
                        //std::cout << "撤单" << i << ":" << t_objOrderInfo.InstrumentID << "|" << t_objOrderInfo.OrderSysID << "|" << t_objOrderInfo.OrderStatus << std::endl;
                        t_tradeapi->MyCancelOrder(t_objOrderInfo.OrderSysID);
                        //如果是平仓报单则继续报入
                        if (t_objOrderInfo.CombOffsetFlag[0] == THOST_FTDC_OF_CloseToday)
                        {
                            time_t nowtime;
                            tm *curtime;
                            nowtime = time(NULL);
                            curtime = localtime(&nowtime);
                            sprintf_s(g_strLog, "----------------------:%d:%d:%d", curtime->tm_hour, curtime->tm_min, curtime->tm_sec);
                            LOG4CPLUS_DEBUG(g_objLogger_DEBUG, g_strLog);
                            switch (t_objOrderInfo.Direction)
                            {
                            case THOST_FTDC_D_Buy:
                            {
                                t_tradeapi->MyOrdering(t_objOrderInfo.InstrumentID, ORDER_DIRECTION_BUY, ORDER_OFFSETFLAG_OFFSET_TODAY, ORDER_AGAINSTPRICE, t_objOrderInfo.VolumeTotal, t_marketapi->getCurrentPrice(t_objOrderInfo.InstrumentID)->AskPrice1);
                            }
                            case THOST_FTDC_D_Sell:
                            {
                                t_tradeapi->MyOrdering(t_objOrderInfo.InstrumentID, ORDER_DIRECTION_SELL, ORDER_OFFSETFLAG_OFFSET_TODAY, ORDER_AGAINSTPRICE, t_objOrderInfo.VolumeTotal, t_marketapi->getCurrentPrice(t_objOrderInfo.InstrumentID)->BidPrice1);
                            }
                            }
                        }
                    }
                    break;
                }
                //不需要撤单
                case THOST_FTDC_OST_AllTraded:
                case THOST_FTDC_OST_PartTradedNotQueueing:
                case THOST_FTDC_OST_NoTradeNotQueueing:
                default: break;
                }
            }
            Sleep(1000);
        }
        LOG4CPLUS_DEBUG(g_objLogger_DEBUG, "CancelOrder finish");
    }
    catch (std::exception e)
    {
        sprintf_s(g_strLog, "CancelOrder:%s", e.what());
        LOG4CPLUS_DEBUG(g_objLogger_DEBUG, g_strLog);
    }
}

//止损止盈平仓线程
#ifdef TRADEAPI_VERSION
void Offset(axapi::TradeAPI* t_tradeapi, axapi::MarketQuotationAPI* t_marketapi)
#endif TRADEAPI_VERSION
{
    try
    {
        // 用于获取时间
        time_t nowtime;
        tm *curtime;
        int curMinutes, t_TradeInfoTime;
        char t_TradeInfoHour[3];
        char t_TradeInfoMinutes[3];
        char t_TradeInfoSeconds[3];
        // 合约最小变动价位
        APINamespace CThostFtdcInstrumentField t_objInstrumentInfo;
        // 当前合约行情
        axapi::MarketDataField t_objCurrentPrice;
        // 是否下平仓单标志
        bool t_offsetAction = false;
        // 平仓单类型
        char t_offsettype[10] = "";
        // 下单开平标志,交易所对于平今指令不同
        int t_offsetflag;

        while (g_blOffsetFlag)
        {
            curtime = NULL;
            //gmutex.lock();
            int t_tradesize = t_tradeapi->sizeTradeList();
            sprintf_s(g_strLog, "Offset:sizeTradeList=%d", t_tradesize);
            LOG4CPLUS_DEBUG(g_objLogger_DEBUG, g_strLog);
            for (int i = 1; i <= t_tradesize&&g_blOffsetFlag; i++)
            {
                //std::cout << "Offset:" << i << endl;
                axapi::TradeField t_objTradeInfo = t_tradeapi->getTradeInfo(i);
                if (strcmp(t_objTradeInfo.apiTradeField.InstrumentID, "") == 1 && t_objTradeInfo.Volumn > 0)
                {
                    axapi::MarketDataField* t_currentPrice = t_marketapi->getCurrentPrice(t_objTradeInfo.apiTradeField.InstrumentID);
                    // 没行情暂时不处理
                    if (t_currentPrice == NULL)
                    {
                        continue;
                    }
                    else
                    {
                        memcpy_s(&t_objCurrentPrice, sizeof(t_objCurrentPrice), t_currentPrice, sizeof(t_objCurrentPrice));
                    }
                    // 如果为平仓记录则跳过
                    if (t_objTradeInfo.apiTradeField.OffsetFlag != THOST_FTDC_OF_Open)
                    {
                        continue;
                    }

                    // 更新最高价
                    if ((t_objTradeInfo.apiTradeField.Direction == THOST_FTDC_D_Buy
                        && t_objTradeInfo.Price < t_objCurrentPrice.LastPrice)
                        || (t_objTradeInfo.apiTradeField.Direction == THOST_FTDC_D_Sell
                            && t_objTradeInfo.Price > t_objCurrentPrice.LastPrice))
                    {
                        t_tradeapi->setTradeInfo(i, "Price", t_objCurrentPrice.LastPrice);
                        t_objTradeInfo.Price = t_objCurrentPrice.LastPrice;
                    }

                    /*
                    * TODO:是否强平
                    * 0.达到预期盈利平仓
                    * 1.触碰均线平仓
                    */
#pragma region
                    memcpy_s(&t_objInstrumentInfo, sizeof(t_objInstrumentInfo), &t_tradeapi->getInstrumentInfo(t_objTradeInfo.apiTradeField.InstrumentID), sizeof(t_objInstrumentInfo));
                    double t_dbMA = t_objCurrentPrice.Volume == 0 ? 0 : (t_objCurrentPrice.Turnover / t_objCurrentPrice.Volume / t_objInstrumentInfo.VolumeMultiple);
                    /// 达到预期盈利平仓
                    if ((t_objTradeInfo.Price - t_objTradeInfo.apiTradeField.Price)
                        *(t_objTradeInfo.apiTradeField.Direction == THOST_FTDC_D_Buy ? 1 : -1)
                        - (g_nProfitPoint2Offset *  t_objInstrumentInfo.PriceTick) > 0)
                    {
                        sprintf_s(t_offsettype, 9, (t_objTradeInfo.apiTradeField.Direction == THOST_FTDC_D_Buy ? "byprofit" : "slprofit"));
                        t_offsetAction = true;
                    }
                    /// 触碰均线平仓
                    else if ((t_objTradeInfo.Price - t_dbMA)
                        * (t_objCurrentPrice.LastPrice - t_dbMA) < 0)
                    {
                        sprintf_s(t_offsettype, 9, (t_objTradeInfo.apiTradeField.Direction == THOST_FTDC_D_Buy ? "bytchma" : "sltchma"));
                        t_offsetAction = true;
                    }
#pragma endregion

                    /*
                    * 强平动作
                    */
                    if (t_offsetAction)
                    {
                        if (strcmp(t_objTradeInfo.apiTradeField.ExchangeID, "SHFE") == 0)
                        {
                            t_offsetflag = ORDER_OFFSETFLAG_OFFSET_TODAY;
                        }
                        else
                        {
                            t_offsetflag = ORDER_OFFSETFLAG_OFFSET;
                        }
                        switch (t_objTradeInfo.apiTradeField.Direction)
                        {
                        case THOST_FTDC_D_Buy:
                        {
                            // 买开仓止损
                            sprintf_s(g_strLog, 500, "卖平仓单:%s", t_offsettype);
                            LOG4CPLUS_DEBUG(g_objLogger_DEBUG, g_strLog);
                            if (t_tradeapi->MyOrdering(t_objTradeInfo.apiTradeField.InstrumentID, ORDER_DIRECTION_SELL, t_offsetflag, ORDER_AGAINSTPRICE, t_objTradeInfo.Volumn, t_currentPrice->BidPrice1) >= 0)
                            {
                                t_tradeapi->setTradeInfo(i, "Volumn", 0, 0);
                            }
                        }
                        break;
                        case THOST_FTDC_D_Sell:
                        {
                            // 卖开仓止损
                            sprintf_s(g_strLog, 500, "买平仓单:%s", t_offsettype);
                            LOG4CPLUS_DEBUG(g_objLogger_DEBUG, g_strLog);
                            if (t_tradeapi->MyOrdering(t_objTradeInfo.apiTradeField.InstrumentID, ORDER_DIRECTION_BUY, t_offsetflag, ORDER_AGAINSTPRICE, t_objTradeInfo.Volumn, t_currentPrice->AskPrice1) >= 0)
                            {
                                t_tradeapi->setTradeInfo(i, "Volumn", 0, 0);
                            }
                        }
                        break;
                        }
                    }
                }
            }
            //gmutex.unlock();
            Sleep(1000);
        }
        LOG4CPLUS_DEBUG(g_objLogger_DEBUG, "Offset finish");
    }
    catch (std::exception e)
    {
        sprintf_s(g_strLog, "Offset:%s", e.what());
        LOG4CPLUS_DEBUG(g_objLogger_DEBUG, g_strLog);
    }
}

//全部平仓线程
#ifdef TRADEAPI_VERSION
void OffsetALL(axapi::TradeAPI* t_tradeapi, axapi::MarketQuotationAPI* t_marketapi)
#endif TRADEAPI_VERSION
{
    try
    {
        while (!g_blShutdownFlag)
        {
            // 清盘标志开启,开始清盘操作
            if (g_blOffsetALLFlag)
            {
                g_blCancelFlag = false;
                //gmutex.lock();
                /*
                int t_tradesize = t_tradeapi->sizeTradeList();
                std::cout << "OffsetALL:sizeTradeList=" << t_tradesize << std::endl;
                for(int i=1;i<=t_tradesize;i++)
                {
                TradeField t_objTradeInfo = t_tradeapi->getTradeInfo(i);
                if(strcmp(t_objTradeInfo.apiTradeField.InstrumentID, "") == 1 && t_objTradeInfo.Volumn > 0)
                {
                MarketDataField* t_currentPrice = t_marketapi->getCurrentPrice(t_objTradeInfo.apiTradeField.InstrumentID);
                // 没行情暂时不处理
                if(t_currentPrice == NULL)
                {
                continue;
                }
                // 如果为平仓行情则跳过
                if(t_objTradeInfo.apiTradeField.OffsetFlag != THOST_FTDC_OF_Open)
                {
                continue;
                }
                switch(t_objTradeInfo.apiTradeField.Direction)
                {
                case THOST_FTDC_D_Buy:
                {
                std::cout << "清仓:卖平仓单" << t_objTradeInfo.apiTradeField.InstrumentID << "|" << t_objTradeInfo.Volumn << "|" << t_currentPrice->LastPrice << "|" << t_currentPrice->AskPrice1 << "|" << t_currentPrice->BidPrice1 << std::endl;
                if(t_tradeapi->MyOrdering(t_objTradeInfo.apiTradeField.InstrumentID, ORDER_DIRECTION_SELL, ORDER_OFFSETFLAG_OFFSET_TODAY, ORDER_AGAINSTPRICE, t_objTradeInfo.Volumn, t_currentPrice->BidPrice1) >= 0)
                {
                t_tradeapi->setTradeInfo(i, "Volumn", 0, 0);
                }

                }
                break;
                case THOST_FTDC_D_Sell:
                {
                std::cout << "清仓:买平仓单" << t_objTradeInfo.apiTradeField.InstrumentID << "|" << t_objTradeInfo.Volumn << "|" << t_currentPrice->LastPrice << "|" << t_currentPrice->AskPrice1 << "|" << t_currentPrice->BidPrice1 << std::endl;
                if(t_tradeapi->MyOrdering(t_objTradeInfo.apiTradeField.InstrumentID, ORDER_DIRECTION_BUY, ORDER_OFFSETFLAG_OFFSET_TODAY, ORDER_AGAINSTPRICE, t_objTradeInfo.Volumn, t_currentPrice->AskPrice1) >= 0)
                {
                t_tradeapi->setTradeInfo(i, "Volumn", 0, 0);
                }
                }
                break;
                }
                }
                }*/
                /// 通过当前持仓明细进行平仓操作
                int t_nRequestID = t_tradeapi->queryCustSTKHoldDetail();
                while (!t_tradeapi->checkCompletedQueryRequestID(t_nRequestID))
                {
                    std::cout << "waiting for position detail infomation" << std::endl;
                    Sleep(1000);
                }
                int t_positiondetailsize = t_tradeapi->sizePositionDetailList();
                std::cout << "OffsetALL:sizePositionDetailList=" << t_positiondetailsize << std::endl;
                for (int i = 1; i <= t_positiondetailsize; i++)
                {
                    APINamespace CThostFtdcInvestorPositionDetailField t_objPositionDetailInfo = t_tradeapi->getPositionDetailInfo(i);
                    std::cout << "strcmp(" << t_objPositionDetailInfo.InstrumentID << ", '')=" << strcmp(t_objPositionDetailInfo.InstrumentID, "") << ";Volumn:" << t_objPositionDetailInfo.Volume << std::endl;
                    if (strcmp(t_objPositionDetailInfo.InstrumentID, "") == 1 && t_objPositionDetailInfo.Volume > 0)
                    {
                        axapi::MarketDataField* t_currentPrice = t_marketapi->getCurrentPrice(t_objPositionDetailInfo.InstrumentID);
                        // 没行情暂时不处理
                        if (t_currentPrice == NULL)
                        {
                            continue;
                        }
                        switch (t_objPositionDetailInfo.Direction)
                        {
                        case THOST_FTDC_D_Buy:
                        {
                            std::cout << "清仓:卖平仓单" << t_objPositionDetailInfo.InstrumentID << "|" << t_objPositionDetailInfo.Volume << "|" << t_currentPrice->LastPrice << "|" << t_currentPrice->AskPrice1 << "|" << t_currentPrice->BidPrice1 << std::endl;
                            if (t_tradeapi->MyOrdering(t_objPositionDetailInfo.InstrumentID, ORDER_DIRECTION_SELL, ORDER_OFFSETFLAG_OFFSET_TODAY, ORDER_AGAINSTPRICE, t_objPositionDetailInfo.Volume, t_currentPrice->BidPrice1) >= 0)
                            {
                            }

                        }
                        break;
                        case THOST_FTDC_D_Sell:
                        {
                            std::cout << "清仓:买平仓单" << t_objPositionDetailInfo.InstrumentID << "|" << t_objPositionDetailInfo.Volume << "|" << t_currentPrice->LastPrice << "|" << t_currentPrice->AskPrice1 << "|" << t_currentPrice->BidPrice1 << std::endl;
                            if (t_tradeapi->MyOrdering(t_objPositionDetailInfo.InstrumentID, ORDER_DIRECTION_BUY, ORDER_OFFSETFLAG_OFFSET_TODAY, ORDER_AGAINSTPRICE, t_objPositionDetailInfo.Volume, t_currentPrice->AskPrice1) >= 0)
                            {
                            }
                        }
                        break;
                        }
                    }
                }
                g_blShutdownFlag = true;
                //gmutex.unlock();
                return;
            }
            Sleep(1000);
            LOG4CPLUS_DEBUG(g_objLogger_DEBUG, "OffsetAll loop");
        }
    }
    catch (std::exception e)
    {
        sprintf_s(g_strLog, "OffsetALL:%s", e.what());
        LOG4CPLUS_DEBUG(g_objLogger_DEBUG, g_strLog);
    }
}

int main()
{
    log4cplus::initialize();
    log4cplus::helpers::LogLog::getLogLog()->setInternalDebugging(true);
    g_root = log4cplus::Logger::getRoot();
    g_objLogger_DEBUG = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("debug"));
    g_objLogger_INFO = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("info"));
    try {
        log4cplus::ConfigureAndWatchThread configureThread(
            LOG4CPLUS_TEXT("log4cplus.properties"), 5 * 1000);
    }
    catch (std::exception e) {
        LOG4CPLUS_FATAL(g_root, "initialLog exception");
        return 0;
    }

    /*
    * 参数定义
    */
    // 通用参数定义与赋值
#pragma region
    char INI_FILE[] = "config.ini";
    char t_TradeServerADDR[100], t_MDServerAddr[100], t_CustNo[100], t_CustPass[100], t_BrokerNO[100], t_SleepTime[100];
    char t_OrderStyle[10], t_CurrentPricePremium[100], t_OffsetAllTime[100], t_CancelWaitSeconds[100];
    char t_chInstrument[17];
    int t_nSleepTime, t_nCurrentPricePremium, t_nOffsetAllTime;
#ifdef KSV6T_TRADEAPI
    GetConfigString(INI_FILE, "KSV6TFRONT", t_TradeServerADDR, sizeof(t_TradeServerADDR));
    GetConfigString(INI_FILE, "KSV6TFRONT", t_MDServerAddr, sizeof(t_MDServerAddr));
    GetConfigString(INI_FILE, "USERID", t_CustNo, sizeof(t_CustNo));
    GetConfigString(INI_FILE, "PASSWORD", t_CustPass, sizeof(t_CustPass));
    GetConfigString(INI_FILE, "BROKERNO", t_BrokerNO, sizeof(t_BrokerNO));
#else
#ifdef CTP_TRADEAPI
    GetConfigString(INI_FILE, "CTPFRONT", t_TradeServerADDR, sizeof(t_TradeServerADDR));
    GetConfigString(INI_FILE, "CTPMDFRONT", t_MDServerAddr, sizeof(t_MDServerAddr));
    GetConfigString(INI_FILE, "CTPUSERID", t_CustNo, sizeof(t_CustNo));
    GetConfigString(INI_FILE, "CTPPASSWORD", t_CustPass, sizeof(t_CustPass));
    GetConfigString(INI_FILE, "CTPBROKERNO", t_BrokerNO, sizeof(t_BrokerNO));
#endif CTP_TRADEAPI
#endif KSV6T_TRADEAPI
    GetConfigString(INI_FILE, "CANCELWAITSECONDS", t_CancelWaitSeconds, sizeof(t_CancelWaitSeconds));
    GetConfigString(INI_FILE, "SLEEPTIME", t_SleepTime, sizeof(t_SleepTime));
    GetConfigString(INI_FILE, "ORDERSTYLE", t_OrderStyle, sizeof(t_OrderStyle));
    GetConfigString(INI_FILE, "CURRENTPRICE_PREMIUM", t_CurrentPricePremium, sizeof(t_CurrentPricePremium));
    GetConfigString(INI_FILE, "OFFSETALLMINUTES", t_OffsetAllTime, sizeof(t_OffsetAllTime));
    GetConfigString(INI_FILE, "Instrument", t_chInstrument, sizeof(t_chInstrument));
    t_nSleepTime = atoi(t_SleepTime) * 1000;
    t_nCurrentPricePremium = atoi(t_CurrentPricePremium);
    t_nOffsetAllTime = atoi(t_OffsetAllTime);
    g_nCancelWaitSeconds = atoi(t_CancelWaitSeconds);
#pragma endregion

    // TODO:策略参数定义与赋值
#pragma region
    char t_ProfitPoint2Offset[100];
    GetConfigString(INI_FILE, "Strategy0311_ProfitPoint2Offset", t_ProfitPoint2Offset, sizeof(t_ProfitPoint2Offset));
    g_nProfitPoint2Offset = atoi(t_ProfitPoint2Offset);
#pragma endregion

    sprintf_s(g_strLog, "ServerAddr:%s/%s\n\
                        USER:%s/%s\n\
                        SleepTime:%d\n\
                        CancelWaitSeconds:%d\n\
                        OrderStyle:%s\n\
                        CurrentPricePremium:%d\n\
                        OffsetAllTime:14:%d\n\
                        Instrument:%s",
        t_TradeServerADDR, t_BrokerNO,
        t_CustNo, t_CustPass,
        t_nSleepTime,
        g_nCancelWaitSeconds,
        t_OrderStyle,
        t_nCurrentPricePremium,
        t_nOffsetAllTime,
        t_chInstrument);
    LOG4CPLUS_INFO(g_objLogger_INFO, g_strLog);

    /*
    * 链接行情网关
    */
#ifdef TRADEAPI_VERSION
    axapi::MarketQuotationAPI* t_marketapi = new axapi::MarketQuotationAPI(t_BrokerNO, t_CustNo, t_CustPass, t_MDServerAddr);
#endif TRADEAPI_VERSION
    /*
    * 连接交易网关
    */
#ifdef TRADEAPI_VERSION
    axapi::TradeAPI* t_tradeapi = new axapi::TradeAPI(t_BrokerNO, t_CustNo, t_CustPass, t_TradeServerADDR);
    //axapi::TradeAPI* t_tradeapi = new axapi::TradeAPI("6C2D786C", "8000100012", "123456", "tcp://10.6.3.183:17993");
#endif TRADEAPI_VERSION

    //    if (strcmp(t_OrderStyle, "PREMIUMP") == 0)
    //    {
    //        int t_queryInstrument = t_tradeapi->queryInstrument();
    //#ifdef TRADEAPI_VERSION
    //        while(!t_tradeapi->checkCompletedQueryRequestID(t_queryInstrument))
    //#endif TRADEAPI_VERSION
    //        {
    //            std::cout << "waiting for instrument info..." << std::endl;
    //            log4cplus::helpers::sleep(1);
    //        }
    //    }

    //std::cout << "t_tradeapi:" << t_tradeapi << ";t_marketapi:" << t_marketapi << std::endl;
    std::thread t_thread1(TradeTimeMonitor, t_nOffsetAllTime);
    std::thread t_thread2(CancelOrder, t_tradeapi, t_marketapi);
    std::thread t_thread3(Offset, t_tradeapi, t_marketapi);
    std::thread t_thread4(OffsetALL, t_tradeapi, t_marketapi);

    char t_bsFlag = ORDER_DIRECTION_BUY;
    char t_eoFlag = ORDER_OFFSETFLAG_OPEN;
    int t_doneQTY = 1;
    double t_donePrice;
    double t_MAPrice;
    bool t_OrderFlag;
    axapi::MarketDataField* t_pLaterPrice = NULL;
    APINamespace CThostFtdcInstrumentField t_objInstrumentInfo;

    /// TODO:策略变量定义
#pragma region
    axapi::MarketDataField t_pFormerPrice1;
    memset(&t_pFormerPrice1, '\0', sizeof(t_pFormerPrice1));
    t_pFormerPrice1.LastPrice = 0;
#pragma endregion

    t_objInstrumentInfo = t_tradeapi->getInstrumentInfo(t_chInstrument);
    t_marketapi->subMarketDataSingle(t_chInstrument);
    ///*
    //* 根据策略下单
    //*/
    try
    {
        /*
        * 下单信息
        */
        while (g_blOpenFlag)
        {
            t_MAPrice = 0;
            t_OrderFlag = false;
            Sleep(t_nSleepTime);
            t_pLaterPrice = t_marketapi->getCurrentPrice(t_chInstrument);
            if (t_pLaterPrice != NULL)
            {
                /// TODO:策略部分
#pragma region
                memcpy_s(&t_objInstrumentInfo, sizeof(t_objInstrumentInfo), &t_tradeapi->getInstrumentInfo(t_chInstrument), sizeof(t_objInstrumentInfo));
                if (t_pFormerPrice1.LastPrice == 0)
                {
                    t_pFormerPrice1.LastPrice = t_pLaterPrice->LastPrice;
                    t_pFormerPrice1.Volume = t_pLaterPrice->Volume;
                    t_pFormerPrice1.OpenInterest = t_pLaterPrice->OpenInterest;
                    continue;
                }

                // 策略主体，判断方向开仓
                t_MAPrice = t_pLaterPrice->Volume == 0 ? 0 : t_pLaterPrice->Turnover / t_pLaterPrice->Volume / t_objInstrumentInfo.VolumeMultiple;
                sprintf_s(g_strLog, "FormerPrice:%f;LatestPrice2:%f;AveragePrice:%f",
                    t_pFormerPrice1.LastPrice,
                    t_pLaterPrice->LastPrice,
                    t_MAPrice);
                LOG4CPLUS_INFO(g_objLogger_INFO, g_strLog);

                if ((t_pFormerPrice1.LastPrice - t_MAPrice) * (t_pLaterPrice->LastPrice - t_MAPrice) < 0)
                {
                    // 向下穿
                    if ((t_pFormerPrice1.LastPrice - t_MAPrice) > 0)
                    {
                        t_OrderFlag = true;
                        t_bsFlag = ORDER_DIRECTION_SELL;
                    }
                    // 向上穿
                    else if ((t_pFormerPrice1.LastPrice - t_MAPrice) < 0)
                    {
                        t_OrderFlag = true;
                        t_bsFlag = ORDER_DIRECTION_BUY;
                    }
                }

                t_pFormerPrice1.LastPrice = t_pLaterPrice->LastPrice;
                t_pFormerPrice1.Volume = t_pLaterPrice->Volume;
                t_pFormerPrice1.OpenInterest = t_pLaterPrice->OpenInterest;
#pragma endregion

                //开仓主体
                if (t_OrderFlag)
                {
                    if (strcmp(t_OrderStyle, "CURRENTP") == 0)
                    {
                        t_donePrice = t_pLaterPrice->LastPrice;
                        sprintf_s(g_strLog, "%s,%s,%d,%d,%d,%d,%f", t_OrderStyle, t_chInstrument, t_bsFlag, t_eoFlag, ORDER_LIMITPRICE, t_doneQTY, t_donePrice);
                        LOG4CPLUS_INFO(g_objLogger_INFO, g_strLog);
                        if (t_tradeapi->MyOrdering(t_chInstrument, t_bsFlag, t_eoFlag, ORDER_LIMITPRICE, t_doneQTY, t_donePrice) < 0)
                        {
                            LOG4CPLUS_INFO(g_objLogger_INFO, "下单失败");
                        }
                    }
                    else if (strcmp(t_OrderStyle, "AGAINSTP") == 0)
                    {
                        switch (t_bsFlag)
                        {
                        case ORDER_DIRECTION_BUY:t_donePrice = t_pLaterPrice->AskPrice1; break;
                        case ORDER_DIRECTION_SELL:t_donePrice = t_pLaterPrice->BidPrice1; break;
                        default:t_donePrice = 0;
                        }
                        sprintf_s(g_strLog, "%s,%s,%d,%d,%d,%d,%f", t_OrderStyle, t_chInstrument, t_bsFlag, t_eoFlag, ORDER_LIMITPRICE, t_doneQTY, t_donePrice);
                        LOG4CPLUS_INFO(g_objLogger_INFO, g_strLog);
                        if (t_tradeapi->MyOrdering(t_chInstrument, t_bsFlag, t_eoFlag, ORDER_LIMITPRICE, t_doneQTY, t_donePrice) < 0)
                        {
                            LOG4CPLUS_INFO(g_objLogger_INFO, "下单失败");
                        }
                    }
                    else if (strcmp(t_OrderStyle, "ANYP") == 0)
                    {
                        switch (t_bsFlag)
                        {
                        case ORDER_DIRECTION_BUY:t_donePrice = t_pLaterPrice->AskPrice1; break;
                        case ORDER_DIRECTION_SELL:t_donePrice = t_pLaterPrice->BidPrice1; break;
                        default:t_donePrice = 0;
                        }
                        sprintf_s(g_strLog, "%s,%s,%d,%d,%d,%d,%f", t_OrderStyle, t_chInstrument, t_bsFlag, t_eoFlag, ORDER_LIMITPRICE, t_doneQTY, t_donePrice);
                        LOG4CPLUS_INFO(g_objLogger_INFO, g_strLog);
                        if (t_tradeapi->MyOrdering(t_chInstrument, t_bsFlag, t_eoFlag, ORDER_LIMITPRICE, t_doneQTY, t_donePrice) < 0)
                        {
                            LOG4CPLUS_INFO(g_objLogger_INFO, "下单失败");
                        }
                    }
                    else if (strcmp(t_OrderStyle, "PREMIUMP") == 0)
                    {
                        switch (t_bsFlag)
                        {
                        case ORDER_DIRECTION_BUY:t_donePrice = t_pLaterPrice->AskPrice1 + t_tradeapi->getInstrumentInfo(t_chInstrument).PriceTick * t_nCurrentPricePremium; break;
                        case ORDER_DIRECTION_SELL:t_donePrice = t_pLaterPrice->BidPrice1 - t_tradeapi->getInstrumentInfo(t_chInstrument).PriceTick * t_nCurrentPricePremium; break;
                        default:t_donePrice = 0;
                        }
                        sprintf_s(g_strLog, "%s,%s,%d,%d,%d,%d,%f", t_OrderStyle, t_chInstrument, t_bsFlag, t_eoFlag, ORDER_LIMITPRICE, t_doneQTY, t_donePrice);
                        LOG4CPLUS_INFO(g_objLogger_INFO, g_strLog);
                        if (t_tradeapi->MyOrdering(t_chInstrument, t_bsFlag, t_eoFlag, ORDER_LIMITPRICE, t_doneQTY, t_donePrice) < 0)
                        {
                            LOG4CPLUS_INFO(g_objLogger_INFO, "下单失败");
                        }
                    }
                }
            }
        }
        g_blOffsetALLFlag = true;

    }
    catch (std::exception e)
    {
        LOG4CPLUS_DEBUG(g_objLogger_DEBUG, e.what());
    }

    /*
    * 测试止损止盈
    */
#ifdef TEST
#pragma region
    t_marketapi->subMarketDataSingle("rb1811");
    t_marketapi->subMarketDataSingle("rb1901");
    Sleep(1000);
#pragma endregion
#endif TEST


    int i = 1;
    while (!g_blShutdownFlag)
    {
        log4cplus::helpers::sleep(1);
        sprintf_s(g_strLog, "sleep:%d", i++);
        LOG4CPLUS_DEBUG(g_objLogger_DEBUG, g_strLog);
    }
    Sleep(5000);
    delete(t_tradeapi);
    delete(t_marketapi);
    return 0;
}


