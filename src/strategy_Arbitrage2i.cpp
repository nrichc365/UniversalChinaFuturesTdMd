#define CTP_TRADEAPI
#define LOGGER_NAME "strategy_Arbitrage2i"

#include "strategy_Arbitrage2i.h"

int strategy_Arbitrage2i::initializeSubLog()
{
    m_objLoggerSub = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT(LOGGER_NAME));
    return 0;
}

void strategy_Arbitrage2i::myStrategy(bool * ot_blOpenFlag, std::string * ot_strOpenMsg, char * ot_strContract, int * ot_nDirection, int * ot_nOffsetFlag, int * ot_nOrderTypeFlag, int * ot_nOrderAmount, double * ot_dOrderPrice)
{

}

strategy_Arbitrage2i::strategy_Arbitrage2i(std::vector<std::string> in_vInstruments, unsigned int in_nCancelWaitSeconds, unsigned int in_nOrderingCheckWaitMillseconds)
{
    if (initializeSubLog() == 0)
    {
        LOG4CPLUS_FATAL(m_objLoggerSub, "initialize LOG OK");
    }
    else
    {
        LOG4CPLUS_FATAL(m_objLoggerSub, ":initialize LOG ERROR");
    }

    m_nCancelWaitSeconds = in_nCancelWaitSeconds;
    m_nOrderingCheckWaitMillseconds = in_nOrderingCheckWaitMillseconds;
    m_vCombinationPlan.clear();
    m_vCombinationPaired.clear();
    m_hashInstruments.clear();
    m_vInstrumentsCurrentPrice.clear();
    m_vInstrumentsInfo.clear();
    for (int i = 0; i < in_vInstruments.size(); i++)
    {
        m_hashInstruments[in_vInstruments[i]] = NULL;
    }
}

strategy_Arbitrage2i::~strategy_Arbitrage2i()
{
}

int strategy_Arbitrage2i::initializeAPISub(axapi::MarketQuotationAPI *in_pMarketQuotationAPI, axapi::TradeAPI *in_pTradeAPI)
{
    if (this->initializeAPI(in_pMarketQuotationAPI, in_pTradeAPI) == 0)
    {
        /// 订阅需要用到合约的行情
        for (std::hash_map<std::string/*axapi::UniversalChinaFutureTdInstrumentIDType*/, axapi::UniversalChinaFutureTdSequenceType>::iterator i = m_hashInstruments.begin();
            i != m_hashInstruments.end(); i++)
        {
            axapi::UniversalChinaFutureTdInstrumentIDType t_chInsturmentID;
            strcpy_s(t_chInsturmentID, sizeof(t_chInsturmentID), i->first.c_str());
            m_pMarketQuotation->subMarketDataSingle(t_chInsturmentID);
        }

        /// 先取合约信息
        for (std::hash_map<std::string/*axapi::UniversalChinaFutureTdInstrumentIDType*/, axapi::UniversalChinaFutureTdSequenceType>::iterator i = m_hashInstruments.begin();
            i != m_hashInstruments.end(); i++)
        {
            axapi::UniversalChinaFutureTdInstrumentIDType t_chInsturmentID;
            strcpy_s(t_chInsturmentID, sizeof(t_chInsturmentID), i->first.c_str());
            APINamespace CThostFtdcInstrumentField t_objInstrumentField;
            t_objInstrumentField = m_pTrade->getInstrumentInfo(t_chInsturmentID);
            if (t_objInstrumentField.InstrumentID[0] == '\0')
            {
                return -200;
            }
            m_vInstrumentsInfo.push_back(t_objInstrumentField);
            m_hashInstruments[i->first] = m_vInstrumentsInfo.size() - 1;
        }
        /// 后取行情信息
        for (int i = 0; i < m_vInstrumentsInfo.size(); i++)
        {
            unsigned int t_nWaitCount = 0;
            axapi::MarketDataField* t_pCurrentPrice = NULL;
            while (t_pCurrentPrice == NULL)
            {
                t_pCurrentPrice = m_pMarketQuotation->getCurrentPrice(m_vInstrumentsInfo[i].InstrumentID);
                Sleep(100);
                t_nWaitCount++;
                if (t_nWaitCount > 100)
                {
                    return -400;
                }
            }
            m_vInstrumentsCurrentPrice.push_back(t_pCurrentPrice);
            if (i != m_vInstrumentsCurrentPrice.size() - 1)
            {
                return -300;
            }
        }
        return 0;
    }
    else
    {
        return -100;
    }
}
