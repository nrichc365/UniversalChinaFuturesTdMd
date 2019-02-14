#include "StrategyMaxLose.h"

axapi::StrategyMaxLose_OneContract::StrategyMaxLose_OneContract()
{
}

axapi::StrategyMaxLose_OneContract::~StrategyMaxLose_OneContract()
{
}

// 检查下单参数是否全部提供
bool axapi::StrategyMaxLose_OneContract::check(std::vector<std::string> in_parameterList)
{
    return true;
}

// 下单策略
bool axapi::StrategyMaxLose_OneContract::order(axapi::TradeAPI *in_tradeAPI, axapi::MarketQuotationAPI *in_marketquotationAPI, std::vector<std::string> in_parameterList)
{
    /*TODO:使用API接口下单MyOrdering*/
    return true;
}

// 收盘策略
bool axapi::StrategyMaxLose_OneContract::closeMarket(axapi::TradeAPI *in_tradeAPI, axapi::MarketQuotationAPI *in_marketquotationAPI)
{
    /*TODO:当达到收盘条件后调用收盘策略*/
    return true;
}

// 平仓策略
bool axapi::StrategyMaxLose_OneContract::offset(axapi::TradeAPI *in_tradeAPI, axapi::MarketQuotationAPI *in_marketquotationAPI)
{
    /*TODO:使用API接口平仓*/
    return true;
}

// 撤单策略
bool axapi::StrategyMaxLose_OneContract::cancel(axapi::TradeAPI *in_tradeAPI, axapi::MarketQuotationAPI *in_marketquotationAPI)
{
    /*TODO:使用API接口撤单*/
    return true;
}
