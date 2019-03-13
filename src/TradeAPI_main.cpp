//#define CTP_TRADEAPI
#define KSV6T_TRADEAPI
#define MEMORYDATA
//#define SQLITE3DATA
#include "TradeAPI.h"
#include "TradeAPITypeDefine.h"
#include <iostream>

void main()
{
#ifdef KSV6T_TRADEAPI
    axapi::TradeAPI *p_tradeAPI = new axapi::TradeAPI("6C2D786C", "8000100078", "131", "tcp://10.6.3.183:17993");
#endif KSV6T_TRADEAPI
#ifdef CTP_TRADEAPI
    axapi::TradeAPI *p_tradeAPI = new axapi::TradeAPI("4580", "8000500200", "800050", "tcp://10.6.7.196:21205");
    //axapi::TradeAPI *p_tradeAPI = new axapi::TradeAPI("4500", "80005000336", "197913", "tcp://27.115.97.3:41205");
#endif CTP_TRADEAPI

    char *t_strContract = "rb1905";
    int t_nRequestID = p_tradeAPI->queryMarketData(t_strContract);
    while (!p_tradeAPI->checkCompletedQueryRequestID(t_nRequestID))
    {
        Sleep(1000);
    }
    APINamespace CThostFtdcDepthMarketDataField t_MarketData = p_tradeAPI->getLatestPrice();
    long t_OrderRef = 0;
    p_tradeAPI->MyOrdering("rb1905", ORDER_DIRECTION_BUY, ORDER_OFFSETFLAG_OPEN, ORDER_LIMITPRICE, 1, t_MarketData.LowerLimitPrice, &t_OrderRef);

    std::cout << "t_OrderRef:" << t_OrderRef << std::endl;

    // 持仓明细
#pragma region
    /*int t_nRequestID = p_tradeAPI->queryCustSTKHoldDetail();
    while(!p_tradeAPI->checkCompletedQueryRequestID(t_nRequestID) && true)
    {
    std::cout << "waiting for position detail infomation" << std::endl;
    Sleep(1000);
    }*/
#pragma endregion

    // 合约信息
#pragma region
    /*int t_nInstrumentSize = p_tradeAPI->sizeInstrumentList();
    while(true)
    {
    for(int i = 1; i <= t_nInstrumentSize; i++)
    {
    std::cout << p_tradeAPI->getInstrumentInfo(i).InstrumentID << "|" << p_tradeAPI->getInstrumentInfo(i).IsTrading << std::endl;
    }
    Sleep(2000);
    }*/
#pragma endregion

    // 委托信息
#pragma region
    while (true)
    {
        int t_nOrderSize = p_tradeAPI->sizeOrderList();
        std::cout << "t_nOrderSize:" << t_nOrderSize << std::endl;
        for (int i = 1; i <= t_nOrderSize; i++)
        {
            std::cout << p_tradeAPI->getOrderInfo(i).OrderRef << "|"
                << p_tradeAPI->getOrderInfo(i).OrderSysID << "|"
                << p_tradeAPI->getOrderInfo(i).InstrumentID << "|"
                << p_tradeAPI->getOrderInfo(i).OrderStatus
                << std::endl;
        }
        Sleep(2000);
    }
#pragma endregion


    // 普通循环
#pragma region
    while (true)
    {
        Sleep(2000);
    }
#pragma endregion

    exit(0);
}