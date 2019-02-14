#ifndef _Strategy_MaxLose_
#define _Strategy_MaxLose_
#pragma once

#include "StrategyModel.h"

namespace axapi
{
    // 止损策略,但合约
    class StrategyMaxLose_OneContract :
        public StrategyModel
    {
    public:
        StrategyMaxLose_OneContract();
        ~StrategyMaxLose_OneContract();

        // 检查下单参数是否全部提供
        bool check(std::vector<std::string> in_parameterList);
        // 下单策略
        bool order(axapi::TradeAPI *in_tradeAPI, axapi::MarketQuotationAPI *in_marketquotationAPI, std::vector<std::string> in_parameterList);
        // 收盘策略
        bool closeMarket(axapi::TradeAPI *in_tradeAPI, axapi::MarketQuotationAPI *in_marketquotationAPI);
        // 平仓策略
        bool offset(axapi::TradeAPI *in_tradeAPI, axapi::MarketQuotationAPI *in_marketquotationAPI);
        // 撤单策略
        bool cancel(axapi::TradeAPI *in_tradeAPI, axapi::MarketQuotationAPI *in_marketquotationAPI);
    };
}

#endif //_Strategy_MaxLose_
