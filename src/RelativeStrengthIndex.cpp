#define RELATIVESTRENGTHINDEX_EXE
#include "RelativeStrengthIndex.h"
#include <iostream>

axapi::RelativeStrengthIndex::RelativeStrengthIndex(void)
{
    m_pMarketQuotation = NULL;
}


axapi::RelativeStrengthIndex::~RelativeStrengthIndex(void)
{
}

axapi::RelativeStrengthIndex::RelativeStrengthIndex(MarketQuotationAPI* in_pMarketQuotationAPI, int t_n1mKBars)
{
    if(in_pMarketQuotationAPI == NULL)
    {
    }
    else
    {
        m_n1mKBars = t_n1mKBars;
        m_pMarketQuotation = in_pMarketQuotationAPI;
    }
}

/// index计算入口
void axapi::RelativeStrengthIndex::caculate()
{
    /// 判断是否需要增加合约
    double t_ClosePriceSUM_Ascend = 0;
    double t_ClosePriceSUM_Descend = 0;
    double t_ClosePrice = 0;
    axapi::KMarketDataField *t_pMarketDataField;
    for(int i=0;i<m_n1mKBars;i++)
    {
        t_pMarketDataField = m_pMarketQuotation->getKLine("rb1901",1,i*(-1));
        if(t_pMarketDataField != NULL)
        {
            t_ClosePrice = t_pMarketDataField->ClosePrice;
            std::cout << i << ":" << t_pMarketDataField->ClosePrice << "-" << t_pMarketDataField->OpenPrice << std::endl;
            if(t_pMarketDataField->ClosePrice-t_pMarketDataField->OpenPrice >= 0)
                t_ClosePriceSUM_Ascend += t_pMarketDataField->ClosePrice-t_pMarketDataField->OpenPrice;
            else
                t_ClosePriceSUM_Descend += t_pMarketDataField->OpenPrice-t_pMarketDataField->ClosePrice;
        }
    }
    if (t_ClosePriceSUM_Ascend+t_ClosePriceSUM_Descend == 0)
    {
        m_nIndexValue = 0;
    }
    else
    {
        m_nIndexValue = t_ClosePriceSUM_Ascend / (t_ClosePriceSUM_Ascend + t_ClosePriceSUM_Descend) * 100;
    }
}

double axapi::RelativeStrengthIndex::getIndexValue()
{
    return m_nIndexValue;
}