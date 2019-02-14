#ifndef _STRATEGY_MODEL_
#define _STRATEGY_MODEL_
#pragma once

#include <string>
#include <vector>
#include <../src/TradeAPI.h>
#include <../src/MarketQuotationAPI.h>

namespace axapi
{
    // 策略接口
    class StrategyModel
    {
    public:
        StrategyModel() {};
        ~StrategyModel() {};

        // 检查下单参数是否全部提供
        virtual bool check(std::vector<std::string> in_parameterList) = 0;
        // 下单策略
        virtual bool order(axapi::TradeAPI *in_tradeAPI, axapi::MarketQuotationAPI *in_marketquotationAPI, std::vector<std::string> in_parameterList) = 0;
        // 收盘策略
        virtual bool closeMarket(axapi::TradeAPI *in_tradeAPI, axapi::MarketQuotationAPI *in_marketquotationAPI) = 0;
        // 平仓策略
        virtual bool offset(axapi::TradeAPI *in_tradeAPI, axapi::MarketQuotationAPI *in_marketquotationAPI) = 0;
        // 撤单策略
        virtual bool cancel(axapi::TradeAPI *in_tradeAPI, axapi::MarketQuotationAPI *in_marketquotationAPI) = 0;
    };
}
#endif //_STRATEGY_MODEL_