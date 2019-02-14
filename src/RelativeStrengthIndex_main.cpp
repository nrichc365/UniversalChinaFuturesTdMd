#define RELATIVESTRENGTHINDEX_EXE
#include "RelativeStrengthIndex.h"
//#define CTP_TRADEAPI
//#define KLINESTORAGE
//#include <MarketQuotationAPI\MarketQuotationAPI.h>
#include <iostream>

void main()
{
    //axapi::MarketQuotationAPI* t_pMarketQuotation = new axapi::MarketQuotationAPI("6C2D786C", "8000100078", "131", "tcp://10.6.3.183:17993");
    axapi::MarketQuotationAPI* t_pMarketQuotation = new axapi::MarketQuotationAPI("4500", "21001", "gtax@1", "tcp://10.6.7.80:41213");
    t_pMarketQuotation->subMarketDataSingle("rb1901");
    /*std::cout << "1" << std::endl;
    while(true)
    {
        Sleep(1000);
        axapi::KMarketDataField *t_pMarketDataField = t_pMarketQuotation->getKLine("rb1901",1,0);
        std::cout << t_pMarketDataField->ClosePrice << std::endl;
    }*/

    axapi::RelativeStrengthIndex* t_pRSI = new axapi::RelativeStrengthIndex(t_pMarketQuotation, 7);
    while(true)
    {
        t_pRSI->caculate();
        std::cout << t_pRSI->getIndexValue() << std::endl;
        Sleep(20000);
    }
}