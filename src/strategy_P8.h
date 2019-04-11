#ifndef _STRATEGY_P8_H_
#define _STRATEGY_P8_H_
#pragma once

#include <Strategy.h>

class strategy_P8 :
    public axapi::Strategy
{
private:
    axapi::MarketDataField m_objFormerPrice1, m_objFormerPrice2;
    APINamespace CThostFtdcInstrumentField m_objInstrumentInfo;
    axapi::MarketDataField *m_pCurrentPrice;
    axapi::UniversalChinaFutureTdInstrumentIDType m_chInstrument;

    /// 策略下单配置
    char m_chStrategy_PPP[5];
    char m_chStrategy_PPN[5];
    char m_chStrategy_PNP[5];
    char m_chStrategy_PNN[5];
    char m_chStrategy_NPP[5];
    char m_chStrategy_NPN[5];
    char m_chStrategy_NNP[5];
    char m_chStrategy_NNN[5];
    int m_nOffsetInterval;
    /// 止损价位
    int m_nOffsetPriceDiff;
    /// 止盈价位
    int m_nOffsetPriceDiff2;
    /// 回撤强平策略阀值
    int m_nProfitFallOffsetValve;
    /// 回撤强平策略最大允许回撤率
    double m_dbProfitFallRate;

    /// 策略日志初始化
    int initializeSubLog();

protected:
    /// TODO:策略主体
    void myStrategy(bool *ot_blOpenFlag, std::string *ot_strOpenMsg, char *ot_strContract, int *ot_nDirection, int *ot_nOffsetFlag, int *ot_nOrderTypeFlag, int *ot_nOrderAmount, double *ot_dOrderPrice);
    /// TODO:平仓主体
    void myOffsetStrategy(struct axapi::ConfirmedHoldTrade in_objHoldTrade, bool *ot_blOffsetFlag, std::string *ot_strOffsetMsg);
    /// TODO:预埋单价位获得
    void getPreOffsetPrice(struct axapi::ConfirmedHoldTrade in_objHoldTrade, bool *ot_blSPOffsetFlag, double *ot_dbSPOffsetPrice);
public:
    strategy_P8(char *in_chInstrument, int in_nCancelWaitSeconds, char *in_chPPPDirection, char *in_chPPNDirection, char *in_chPNPDirection, char *in_chPNNDirection, char *in_chNPPDirection, char *in_chNPNDirection, char *in_chNNPDirection, char *in_chNNNDirection, int in_nOffsetInterval = INT_MAX, int in_nOffsetPriceDiff = INT_MAX, int in_nOffsetPriceDiff2 = INT_MAX, int in_nProfitFallOffsetValve = INT_MAX, double in_dbProfitFallRate = 1);
    ~strategy_P8();
};

#endif _STRATEGY_P8_H_
