#ifndef _STRATEGY_MA_H_
#define _STRATEGY_MA_H_
#pragma once

#include <Strategy.h>

class strategy_MA :
    public axapi::Strategy
{
private:
    APINamespace CThostFtdcInstrumentField m_objInstrumentInfo;
    axapi::MarketDataField *m_pCurrentPrice;
    axapi::UniversalChinaFutureTdInstrumentIDType m_chInstrument;

    /// 策略下单配置
    axapi::MarketDataField m_objFormerPrice1;
    unsigned int m_nProfitPoint2Offset;

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
    strategy_MA(char *in_chInstrument, unsigned int in_nCancelWaitSeconds, unsigned int in_nOrderingCheckWaitMillseconds, unsigned int in_nProfitPoint2Offset);
    ~strategy_MA();
    /// 初始化行情与交易接口
    int initializeAPISub(axapi::MarketQuotationAPI*, axapi::TradeAPI*);
};

#endif _STRATEGY_MA_H_
