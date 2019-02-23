#define CTP_TRADEAPI
//#define KSV6T_TRADEAPI
//#define DATAFILE
#define KLINESTORAGE
#include "MarketQuotationAPI.h"
#include <iostream>

void main()
{
#ifdef KSV6T_TRADEAPI
    axapi::MarketQuotationAPI *pmarketquotationapi = new axapi::MarketQuotationAPI("6C2D786C", "8000100078", "131", "tcp://10.6.3.183:17993");
#endif KSV6T_TRADEAPI
#ifdef CTP_TRADEAPI
    axapi::MarketQuotationAPI *pmarketquotationapi = new axapi::MarketQuotationAPI("4500", "21001", "gtax@1", "tcp://10.6.7.80:41213");
#endif CTP_TRADEAPI
    //Sleep(1000);
    std::cout << "123" << std::endl;
    pmarketquotationapi->subMarketDataSingle("AP1905");
    std::cout << "234" << std::endl;
    //pmarketquotationapi->subMarketDataSingle("rb1902");
    //pmarketquotationapi->subMarketDataSingle("rb1903");
    //pmarketquotationapi->subMarketDataSingle("rb1904");
    //pmarketquotationapi->subMarketDataSingle("rb1905");
    //pmarketquotationapi->subMarketDataSingle("rb1906");
    //pmarketquotationapi->subMarketDataSingle("rb1907");
    //pmarketquotationapi->subMarketDataSingle("rb1908");
    //pmarketquotationapi->subMarketDataSingle("rb1909");
    //pmarketquotationapi->subMarketDataSingle("rb1910");
    //Sleep(1000);
    //std::cout << "1mK size:" << pmarketquotationapi->m_array1mKLine.size() << std::endl;
    /*
    for(int i=0; i<pmarketquotationapi->m_array1mKLine.size(); i++)
    {
    for(int j=600; j<610; j++)
    {
    std::cout << "  " << (pmarketquotationapi->m_array1mKLine[i]+j)->TradingDay << ','
    << (pmarketquotationapi->m_array1mKLine[i]+j)->Contract << ','
    << (pmarketquotationapi->m_array1mKLine[i]+j)->SecondsPeriod << ','
    << (pmarketquotationapi->m_array1mKLine[i]+j)->BarSerials << ','
    << (pmarketquotationapi->m_array1mKLine[i]+j)->begintime << ','
    << (pmarketquotationapi->m_array1mKLine[i]+j)->endtime << ','
    << (pmarketquotationapi->m_array1mKLine[i]+j)->OpenPrice << ','
    << (pmarketquotationapi->m_array1mKLine[i]+j)->HighestPrice << ','
    << (pmarketquotationapi->m_array1mKLine[i]+j)->LowestPrice << ','
    << (pmarketquotationapi->m_array1mKLine[i]+j)->ClosePrice << ','
    << (pmarketquotationapi->m_array1mKLine[i]+j)->Volume << ','
    << (pmarketquotationapi->m_array1mKLine[i]+j)->Turnover
    << std::endl;
    }
    }
    */

    axapi::KMarketDataField *t_pmarketquotation1 = NULL, *t_pmarketquotation2 = NULL;
    while(true)
    {

        axapi::MarketDataField* pMarketData = pmarketquotationapi->getCurrentPrice("AP1905");
        std::cout << pmarketquotationapi->getCurrPrice("AP1905") << ":";
        std::cout << pMarketData->InstrumentID << "|" << pMarketData->AskPrice1 << "|" << pMarketData->BidPrice1
            << "|" << pMarketData->Turnover << "|" << pMarketData->OpenInterest << "|" << pMarketData->LastPrice
            << "|" << pMarketData->Volume
            << std::endl;

        /*t_pmarketquotation1 = pmarketquotationapi->getKLine("rb1901",1,0);
        t_pmarketquotation2 = pmarketquotationapi->getKLine("rb1901",1,-1);
        if(t_pmarketquotation1 != NULL)
        std::cout << t_pmarketquotation1->ClosePrice << std::endl;
        if(t_pmarketquotation2 != NULL)
        std::cout << t_pmarketquotation2->ClosePrice << std::endl;*/

        Sleep(1000);
    }
    return;
}