#define CTP_TRADEAPI
//#define KSV6T_TRADEAPI
#include "strategy_P8.h"
#include "configRead_dll/readConfig.h"
#include <iostream>
#include <signal.h>

#define OFFSETALL 'o'

strategy_P8 *pSP8;
bool g_blRunning;
bool g_blWaitShutdown;
void exitsignal(int t_signal);

void main()
{
    pSP8 = NULL;
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

    char t_OffsetPriceDiff[100], t_OffsetPriceDiff2[100], t_ProfitFallOffsetValve[100], t_ProfitFallRate[100];
    char t_strStrategy_PPP[5], t_strStrategy_PPN[5], t_strStrategy_PNP[5], t_strStrategy_PNN[5], t_strStrategy_NPP[5], t_strStrategy_NPN[5], t_strStrategy_NNP[5], t_strStrategy_NNN[5];
    GetConfigString(INI_FILE, "OFFSETPRICEDIFF", t_OffsetPriceDiff, sizeof(t_OffsetPriceDiff));
    GetConfigString(INI_FILE, "OFFSETPRICEDIFF2", t_OffsetPriceDiff2, sizeof(t_OffsetPriceDiff2));
    GetConfigString(INI_FILE, "PROFITFALLOFFSETVALVE", t_ProfitFallOffsetValve, sizeof(t_ProfitFallOffsetValve));
    GetConfigString(INI_FILE, "PROFITFALLRATE", t_ProfitFallRate, sizeof(t_ProfitFallRate));
    GetConfigString(INI_FILE, "STRATEGY_PPP", t_strStrategy_PPP, sizeof(t_strStrategy_PPP));
    GetConfigString(INI_FILE, "STRATEGY_PPN", t_strStrategy_PPN, sizeof(t_strStrategy_PPN));
    GetConfigString(INI_FILE, "STRATEGY_PNP", t_strStrategy_PNP, sizeof(t_strStrategy_PNP));
    GetConfigString(INI_FILE, "STRATEGY_PNN", t_strStrategy_PNN, sizeof(t_strStrategy_PNN));
    GetConfigString(INI_FILE, "STRATEGY_NPP", t_strStrategy_NPP, sizeof(t_strStrategy_NPP));
    GetConfigString(INI_FILE, "STRATEGY_NPN", t_strStrategy_NPN, sizeof(t_strStrategy_NPN));
    GetConfigString(INI_FILE, "STRATEGY_NNP", t_strStrategy_NNP, sizeof(t_strStrategy_NNP));
    GetConfigString(INI_FILE, "STRATEGY_NNN", t_strStrategy_NNN, sizeof(t_strStrategy_NNN));

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

    pSP8 = new strategy_P8(t_chInstrument, atoi(t_CancelWaitSeconds), 1000, t_strStrategy_PPP, t_strStrategy_PPN, t_strStrategy_PNP, t_strStrategy_PNN, t_strStrategy_NPP, t_strStrategy_NPN, t_strStrategy_NNP, t_strStrategy_NNN, atoi(t_SleepTime) * 1000, atoi(t_OffsetPriceDiff), atoi(t_OffsetPriceDiff2), atoi(t_ProfitFallOffsetValve), atof(t_ProfitFallRate));
    if (pSP8->initializeAPISub(t_marketapi, t_tradeapi) == 0)
    {
        pSP8->start();
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
    if (pSP8 != NULL)
    {
        pSP8->stop();
        pSP8 = NULL;
    }
    g_blWaitShutdown = false;
}