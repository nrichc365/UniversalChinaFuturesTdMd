#define CTP_TRADEAPI
//#define KSV6T_TRADEAPI
#include "strategy_MA.h"
#include "configRead_dll/readConfig.h"
#include <iostream>
#include <signal.h>

#define OFFSETALL 'o'

strategy_MA *pSMA;
bool g_blRunning;
bool g_blWaitShutdown;
void exitsignal(int t_signal);

void main()
{
    pSMA = NULL;
    g_blRunning = true;
    g_blWaitShutdown = true;
    signal(SIGINT, exitsignal);
    signal(SIGTERM, exitsignal);
    signal(SIGABRT, exitsignal);

    /*
    * 参数定义
    */
    char t_strLog[500];
    char INI_FILE[] = "config.ini";
    char t_TradeServerADDR[100], t_MDServerAddr[100], t_CustNo[100], t_CustPass[100], t_BrokerNO[100], t_SleepTime[100];
    char t_OrderStyle[10], t_CurrentPricePremium[100], t_OffsetAllTime[100], t_CancelWaitSeconds[100], t_chInstrument[17];
    int t_nOffsetAllTime;
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
    t_nOffsetAllTime = atoi(t_OffsetAllTime);

    char t_chProfitPoint2Offset[100];
    GetConfigString(INI_FILE, "ProfitPoint2Offset", t_chProfitPoint2Offset, sizeof(t_chProfitPoint2Offset));

    signal(SIGINT, exitsignal);
    signal(SIGTERM, exitsignal);
    signal(SIGABRT, exitsignal);

    /*
    * 链接行情网关
    */
    axapi::MarketQuotationAPI* t_marketapi = new axapi::MarketQuotationAPI(t_BrokerNO, t_CustNo, t_CustPass, t_MDServerAddr);
    /*
    * 连接交易网关
    */
    axapi::TradeAPI* t_tradeapi = new axapi::TradeAPI(t_BrokerNO, t_CustNo, t_CustPass, t_TradeServerADDR);

    pSMA = new strategy_MA(t_chInstrument, atoi(t_CancelWaitSeconds), atoi(t_SleepTime) * 1000, atoi(t_chProfitPoint2Offset));
    if (pSMA->initializeAPISub(t_marketapi, t_tradeapi) == 0)
    {
        pSMA->start();
        while (g_blRunning)
        {
            time_t nowtime = time(NULL);
            tm *curtime = localtime(&nowtime);

            /// 14:??-19:59为清盘时间
            if (((curtime->tm_hour >= 14 && curtime->tm_min >= t_nOffsetAllTime) || curtime->tm_hour >= 15) && curtime->tm_hour < 20)
            {
                sprintf_s(t_strLog, "到清盘时间:%d:%d:%d", curtime->tm_hour, curtime->tm_min, curtime->tm_sec);
                std::cout << t_strLog << std::endl;
                exitsignal(OFFSETALL);
            }
            Sleep(1000);
        }
        while (g_blWaitShutdown)
        {
            Sleep(1000);
        }
    }
}

void exitsignal(int t_signal)
{
    g_blRunning = false;
    if (pSMA != NULL)
    {
        pSMA->stop();
        pSMA = NULL;
    }
    g_blWaitShutdown = false;
}