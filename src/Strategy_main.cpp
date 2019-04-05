#define STRATEGY_EXE
#define CTP_TRADEAPI
//#define KSV6T_TRADEAPI
#include "Strategy.h"
#include <iostream>

void main()
{
#ifdef KSV6T_TRADEAPI
    axapi::TradeAPI *p_tradeAPI = new axapi::TradeAPI("6C2D786C", "8000100078", "131", "tcp://10.6.3.183:17993");
    axapi::MarketQuotationAPI *pmarketquotationapi = new axapi::MarketQuotationAPI("6C2D786C", "8000100078", "131", "tcp://10.6.3.183:17993");
#endif KSV6T_TRADEAPI
#ifdef CTP_TRADEAPI
    axapi::MarketQuotationAPI *pmarketquotationapi = new axapi::MarketQuotationAPI("4580", "8000500200", "800050", "tcp://139.129.108.145:21213");
    axapi::TradeAPI *p_tradeAPI = new axapi::TradeAPI("4580", "8000500200", "800050", "tcp://139.129.108.145:21205");
    //axapi::MarketQuotationAPI *pmarketquotationapi = new axapi::MarketQuotationAPI("4500", "21001", "gtax@1", "tcp://10.6.7.80:41213");
    //axapi::TradeAPI *p_tradeAPI = new axapi::TradeAPI("4500", "80005000336", "197913", "tcp://27.115.97.3:41205");
#endif CTP_TRADEAPI

    axapi::Strategy *t_strategy = new axapi::Strategy();
    if (t_strategy->initializeAPI(pmarketquotationapi, p_tradeAPI) == 0)
    {
        /*t_strategy->start();
        for (int i = 0; i < 20; i++)
        {
            Sleep(1000);
        }
        t_strategy->stop();*/
        std::cout << "ok" << std::endl;
    }
    else
    {
        std::cout << "error" << std::endl;
    }
    Sleep(5000);
}