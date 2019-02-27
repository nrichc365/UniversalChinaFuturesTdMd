#include "MQI_RelativeStrengthIndex.h"
#include <iostream>

void main(void)
{
    //axapi::MarketQuotationAPI *t_pMarketQuotation = new axapi::MarketQuotationAPI("4500", "21001", "gtax@1", "tcp://10.6.7.80:41213");
    axapi::MarketQuotationAPI *t_pMarketQuotation = new axapi::MarketQuotationAPI("4580", "8000500200", "800050", "tcp://10.6.7.196:21213");
    t_pMarketQuotation->subMarketDataSingle("rb1905");

    axapi::MQI_RelativeStrengthIndex *t_pMQI_RelativeStrengthIndex = new axapi::MQI_RelativeStrengthIndex();
    t_pMQI_RelativeStrengthIndex->initialize(t_pMarketQuotation, 7, "rb1905");

    double t_dbIndexValue = NULL;
    for (int i = 0; i <= 5; i++)
    {
        t_dbIndexValue = t_pMQI_RelativeStrengthIndex->getIndexValue();
        if (t_dbIndexValue == NULL)
        {
            std::cout << "no index value" << std::endl;
        }
        else
        {
            std::cout << t_dbIndexValue << std::endl;
        }
        Sleep(3000);
    }

    //t_pMQI_RelativeStrengthIndex->initialize(t_pMarketQuotation, 4, "rb1907");
    for (int i = 0; i <= 5; i++)
    {
        t_dbIndexValue = t_pMQI_RelativeStrengthIndex->getIndexValue();
        if (t_dbIndexValue == NULL)
        {
            std::cout << "no index value" << std::endl;
        }
        else
        {
            std::cout << t_dbIndexValue << std::endl;
        }
        Sleep(3000);
    }

}