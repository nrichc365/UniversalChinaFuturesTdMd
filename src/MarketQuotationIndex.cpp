#include "MarketQuotationIndex.h"

axapi::MarketQuotationIndex::MarketQuotationIndex(void)
{
}

axapi::MarketQuotationIndex::~MarketQuotationIndex()
{
}

axapi::MarketQuotationIndex::MarketQuotationIndex(axapi::MarketQuotationAPI *in_pMarketQuotationAPI, int t_n1mKBars)
{
    initialize(in_pMarketQuotationAPI, t_n1mKBars);
}

void axapi::MarketQuotationIndex::initialize(axapi::MarketQuotationAPI *in_pMarketQuotationAPI, int t_n1mKBars)
{
    if (in_pMarketQuotationAPI == NULL)
    {
    }
    else
    {
        m_n1mKBars = t_n1mKBars;
        m_pMarketQuotation = in_pMarketQuotationAPI;
    }
}