#ifndef _STRATEGY_ARBITRAGE2I_H_
#define _STRATEGY_ARBITRAGE2I_H_
#pragma once

#include <Strategy.h>

struct Arbitrage2i_Combination_Plan
{
    axapi::UniversalChinaFutureTdInstrumentIDType Instrument_left;
    axapi::UniversalChinaFutureTdOrderRefType     OrderRef_left;
    std::string                                   CombinationRef_left;
    axapi::UniversalChinaFutureTdInstrumentIDType Instrument_right;
    axapi::UniversalChinaFutureTdOrderRefType     OrderRef_right;
    std::string                                   CombinationRef_right;
    /// 开平仓价差,PriceDiff=instrument_left.currentprice * ordervolumn_left - instrument_right.currentprice * ordervolumn_right
    axapi::UniversalChinaFutureTdPriceType        OpenPriceDiff_Min;
    axapi::UniversalChinaFutureTdPriceType        OpenPriceDiff_Max;
    axapi::UniversalChinaFutureTdPriceType        OffsetPriceDiff_Min;
    axapi::UniversalChinaFutureTdPriceType        OffsetPriceDiff_Max;
};

struct Arbitrage2i_Combination_Paired
{
    axapi::UniversalChinaFutureTdInstrumentIDType Instrument_left;
    axapi::UniversalChinaFutureTdTradeIDType      TradeID_left;
    axapi::UniversalChinaFutureTdOrderRefType     OrderRef_left;
    std::string                                   CombinationRef_left;
    axapi::UniversalChinaFutureTdInstrumentIDType Instrument_right;
    axapi::UniversalChinaFutureTdTradeIDType      TradeID_right;
    axapi::UniversalChinaFutureTdOrderRefType     OrderRef_right;
    std::string                                   CombinationRef_right;
    /// 开平仓价差,PriceDiff=instrument_left.currentprice * ordervolumn_left - instrument_right.currentprice * ordervolumn_right
    axapi::UniversalChinaFutureTdPriceType        OpenPriceDiff_Min;
    axapi::UniversalChinaFutureTdPriceType        OpenPriceDiff_Max;
    axapi::UniversalChinaFutureTdPriceType        OffsetPriceDiff_Min;
    axapi::UniversalChinaFutureTdPriceType        OffsetPriceDiff_Max;
};

class strategy_Arbitrage2i
    : public axapi::Strategy
{
private:
    /// 用于记录需要下单的套利组合
    std::vector<Arbitrage2i_Combination_Plan> m_vCombinationPlan;
    /// 用于记录已配对(下单成功)的套利组合
    std::vector<Arbitrage2i_Combination_Paired> m_vCombinationPaired;
    /// 用于记录合约对应的信息与行情的序列号
    std::hash_map<std::string/*axapi::UniversalChinaFutureTdInstrumentIDType*/, axapi::UniversalChinaFutureTdSequenceType> m_hashInstruments;
    /// 用于记录策略用到的所有合约的当前行情
    std::vector<axapi::MarketDataField*> m_vInstrumentsCurrentPrice;
    /// 用于记录策略用到的所有合约的合约信息
    std::vector<APINamespace CThostFtdcInstrumentField> m_vInstrumentsInfo;

    /// 策略日志初始化
    int initializeSubLog();
protected:
    /// TODO:策略主体
    void myStrategy(bool *ot_blOpenFlag, std::string *ot_strOpenMsg, char *ot_strContract, int *ot_nDirection, int *ot_nOffsetFlag, int *ot_nOrderTypeFlag, int *ot_nOrderAmount, double *ot_dOrderPrice);
    /// TODO:平仓主体
    void myOffsetStrategy(struct axapi::ConfirmedHoldTrade in_objHoldTrade, bool *ot_blOffsetFlag, std::string *ot_strOffsetMsg);
    /// TODO:预埋单价位获得
    void getPreOffsetPrice(struct axapi::ConfirmedHoldTrade in_objHoldTrade, bool *ot_blSPOffsetFlag, double *ot_dbSPOffsetPrice);
    /// TODO:撤单策略主体
    void myCancelStrategy(struct ConfirmedOrder in_objOrder, bool *ot_blCancelFlag, std::string *ot_strCancelMsg);
public:
    strategy_Arbitrage2i(std::vector<std::string> in_vInstruments , unsigned int in_nCancelWaitSeconds, unsigned int in_nOrderingCheckWaitMillseconds);
    ~strategy_Arbitrage2i();
    /// 初始化行情与交易接口
    int initializeAPISub(axapi::MarketQuotationAPI*, axapi::TradeAPI*);
};

#endif _STRATEGY_ARBITRAGE2I_H_