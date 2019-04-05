#define CTP_TRADEAPI
//#define KSV6T_TRADEAPI
#include "strategy_P8.h"
#include "configRead_dll/readConfig.h"

void main()
{
    /*
    * 参数定义
    */
    char INI_FILE[] = "config.ini";
    char t_TradeServerADDR[100], t_MDServerAddr[100], t_CustNo[100], t_CustPass[100], t_BrokerNO[100], t_SleepTime[100];
    char t_OrderStyle[10], t_CurrentPricePremium[100], t_OffsetAllTime[100], t_OffsetPriceDiff[100], t_OffsetPriceDiff2[100], t_CancelWaitSeconds[100];
    char t_ProfitFallOffsetValve[100], t_ProfitFallRate[100];
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
    GetConfigString(INI_FILE, "OFFSETPRICEDIFF", t_OffsetPriceDiff, sizeof(t_OffsetPriceDiff));
    GetConfigString(INI_FILE, "OFFSETPRICEDIFF2", t_OffsetPriceDiff2, sizeof(t_OffsetPriceDiff2));
    GetConfigString(INI_FILE, "PROFITFALLOFFSETVALVE", t_ProfitFallOffsetValve, sizeof(t_ProfitFallOffsetValve));
    GetConfigString(INI_FILE, "PROFITFALLRATE", t_ProfitFallRate, sizeof(t_ProfitFallRate));
    GetConfigString(INI_FILE, "ORDERSTYLE", t_OrderStyle, sizeof(t_OrderStyle));
    GetConfigString(INI_FILE, "CURRENTPRICE_PREMIUM", t_CurrentPricePremium, sizeof(t_CurrentPricePremium));
    GetConfigString(INI_FILE, "OFFSETALLMINUTES", t_OffsetAllTime, sizeof(t_OffsetAllTime));
    GetConfigString(INI_FILE, "Instrument", t_chInstrument, sizeof(t_chInstrument));
    t_nSleepTime = atoi(t_SleepTime) * 1000;
    t_nCurrentPricePremium = atoi(t_CurrentPricePremium);
    t_nOffsetAllTime = atoi(t_OffsetAllTime);
    g_nOffsetPriceDiff = atoi(t_OffsetPriceDiff);
    g_nOffsetPriceDiff2 = atoi(t_OffsetPriceDiff2);
    g_nCancelWaitSeconds = atoi(t_CancelWaitSeconds);
    g_nProfitFallOffsetValve = atoi(t_ProfitFallOffsetValve);
    g_dbProfitFallRate = atof(t_ProfitFallRate);

    sprintf_s(g_strLog, "ServerAddr:%s/%s\n\
                        USER:%s/%s\n\
                        SleepTime:%d\n\
                        CancelWaitSeconds:%d\n\
                        OffsetPriceDiff:%d\n\
                        OrderStyle:%s\n\
                        CurrentPricePremium:%d\n\
                        OffsetAllTime:14:%d\n\
                        Instrument:%s",
        t_TradeServerADDR, t_BrokerNO,
        t_CustNo, t_CustPass,
        t_nSleepTime,
        g_nCancelWaitSeconds,
        g_nOffsetPriceDiff,
        t_OrderStyle,
        t_nCurrentPricePremium,
        t_nOffsetAllTime,
        t_chInstrument);
    LOG4CPLUS_INFO(g_objLogger_INFO, g_strLog);

    char t_strStrategy_PPP[5], t_strStrategy_PPN[5], t_strStrategy_PNP[5], t_strStrategy_PNN[5], t_strStrategy_NPP[5], t_strStrategy_NPN[5], t_strStrategy_NNP[5], t_strStrategy_NNN[5];
    GetConfigString(INI_FILE, "STRATEGY_PPP", t_strStrategy_PPP, sizeof(t_strStrategy_PPP));
    GetConfigString(INI_FILE, "STRATEGY_PPN", t_strStrategy_PPN, sizeof(t_strStrategy_PPN));
    GetConfigString(INI_FILE, "STRATEGY_PNP", t_strStrategy_PNP, sizeof(t_strStrategy_PNP));
    GetConfigString(INI_FILE, "STRATEGY_PNN", t_strStrategy_PNN, sizeof(t_strStrategy_PNN));
    GetConfigString(INI_FILE, "STRATEGY_NPP", t_strStrategy_NPP, sizeof(t_strStrategy_NPP));
    GetConfigString(INI_FILE, "STRATEGY_NPN", t_strStrategy_NPN, sizeof(t_strStrategy_NPN));
    GetConfigString(INI_FILE, "STRATEGY_NNP", t_strStrategy_NNP, sizeof(t_strStrategy_NNP));
    GetConfigString(INI_FILE, "STRATEGY_NNN", t_strStrategy_NNN, sizeof(t_strStrategy_NNN));
    g_nOffsetInterval = atoi(t_SleepTime);

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

    strategy_P8 *pSP8 = new strategy_P8();
    if (pSP8->initializeAPI(pmarketquotationapi, p_tradeAPI) == 0)
    {
        pSP8->start();
        for (int i = 0; i < 20; i++)
        {
            Sleep(1000);
        }
        pSP8->stop();
    }
}