#define CTP_TRADEAPI
#define LOGGER_NAME "strategy_MA"
//#define KSV6T_TRADEAPI
#include "strategy_MA.h"

strategy_MA::strategy_MA(char *in_chInstrument, unsigned int in_nCancelWaitSeconds, unsigned int in_nOrderingCheckWaitMillseconds, unsigned int in_nProfitPoint2Offset)
{
    if (initializeSubLog() == 0)
    {
        LOG4CPLUS_FATAL(m_objLoggerSub, "initialize LOG OK");
    }
    else
    {
        LOG4CPLUS_FATAL(m_objLoggerSub, ":initialize LOG ERROR");
    }

    memset(&m_objFormerPrice1, '\0', sizeof(m_objFormerPrice1));
    m_objFormerPrice1.LastPrice = 0;
    memset(&m_objInstrumentInfo, '\0', sizeof(m_objInstrumentInfo));
    m_pCurrentPrice = NULL;
    strcpy_s(m_chInstrument, sizeof(m_chInstrument), in_chInstrument);
    m_nCancelWaitSeconds = in_nCancelWaitSeconds;
    m_nOrderingCheckWaitMillseconds = in_nOrderingCheckWaitMillseconds;
    m_nProfitPoint2Offset = in_nProfitPoint2Offset;
}

strategy_MA::~strategy_MA()
{
}

int strategy_MA::initializeAPISub(axapi::MarketQuotationAPI* in_pMarketQuotationAPI, axapi::TradeAPI* in_pTradeAPI)
{
    if (this->initializeAPI(in_pMarketQuotationAPI, in_pTradeAPI) == 0)
    {
        m_pMarketQuotation->subMarketDataSingle(m_chInstrument);
        while (m_objInstrumentInfo.InstrumentID[0] == '\0')
        {
            m_objInstrumentInfo = m_pTrade->getInstrumentInfo(m_chInstrument);
        }
        while (m_pCurrentPrice == NULL)
        {
            m_pCurrentPrice = m_pMarketQuotation->getCurrentPrice(m_chInstrument);
        }
        return 0;
    }
    else
    {
        return -100;
    }
}

int strategy_MA::initializeSubLog()
{
    m_objLoggerSub = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT(LOGGER_NAME));
    return 0;
}

void strategy_MA::myStrategy(bool *ot_blOpenFlag, std::string *ot_strOpenMsg, char *ot_strContract, int *ot_nDirection, int *ot_nOffsetFlag, int *ot_nOrderTypeFlag, int *ot_nOrderAmount, double *ot_dOrderPrice)
{
    char* t_strLogFuncName = "strategy_MA::myStrategy";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);

    *ot_blOpenFlag = false;

    if (m_pCurrentPrice != NULL)
    {
        double t_MAPrice = 0;
        axapi::MarketDataField t_objCurrentPrice;
        memcpy_s(&t_objCurrentPrice, sizeof(t_objCurrentPrice), m_pCurrentPrice, sizeof(t_objCurrentPrice));

        if (m_objFormerPrice1.LastPrice == 0)
        {
            m_objFormerPrice1.LastPrice = t_objCurrentPrice.LastPrice;
            m_objFormerPrice1.Volume = t_objCurrentPrice.Volume;
            m_objFormerPrice1.OpenInterest = t_objCurrentPrice.OpenInterest;
            return;
        }

        // 策略主体，判断方向开仓
        t_MAPrice = t_objCurrentPrice.Volume == 0 ? 0 : t_objCurrentPrice.Turnover / t_objCurrentPrice.Volume / m_objInstrumentInfo.VolumeMultiple;
        sprintf_s(t_strLog, "%s:FormerPrice:%f;LatestPrice2:%f;AveragePrice:%f", t_strLogFuncName,
            m_objFormerPrice1.LastPrice,
            t_objCurrentPrice.LastPrice,
            t_MAPrice);
        LOG4CPLUS_INFO(m_objLoggerSub, t_strLog);

        if ((m_objFormerPrice1.LastPrice - t_MAPrice) * (t_objCurrentPrice.LastPrice - t_MAPrice) < 0)
        {
            // 向下穿
            if ((m_objFormerPrice1.LastPrice - t_MAPrice) > 0)
            {
                *ot_blOpenFlag = true;
                *ot_strOpenMsg = "upward break MA";
                *ot_nDirection = ORDER_DIRECTION_SELL;
            }
            // 向上穿
            else if ((m_objFormerPrice1.LastPrice - t_MAPrice) < 0)
            {
                *ot_blOpenFlag = true;
                *ot_strOpenMsg = "downward break MA";
                *ot_nDirection = ORDER_DIRECTION_BUY;
            }
        }

        m_objFormerPrice1.LastPrice = t_objCurrentPrice.LastPrice;
        m_objFormerPrice1.Volume = t_objCurrentPrice.Volume;
        m_objFormerPrice1.OpenInterest = t_objCurrentPrice.OpenInterest;

        if (*ot_blOpenFlag == true)
        {
            *ot_nOrderAmount = 1;
            *ot_nOffsetFlag = ORDER_OFFSETFLAG_OPEN;
            *ot_nOrderTypeFlag = ORDER_LIMITPRICE;
            ot_strContract = m_chInstrument;
            switch (*ot_nDirection)
            {
            case ORDER_DIRECTION_BUY:*ot_dOrderPrice = t_objCurrentPrice.AskPrice1; break;
            case ORDER_DIRECTION_SELL:*ot_dOrderPrice = t_objCurrentPrice.BidPrice1; break;
            default:*ot_dOrderPrice = 0;
            }
        }
    }
}

void strategy_MA::myOffsetStrategy(axapi::ConfirmedHoldTrade in_objHoldTrade, bool *ot_blOffsetFlag, std::string *ot_strOffsetMsg)
{
    char* t_strLogFuncName = "strategy_MA::myOffsetStrategy";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);

    *ot_blOffsetFlag = false;
    /*
    * TODO:是否强平
    * 0.达到预期盈利平仓
    * 1.触碰均线平仓
    */
    if (m_pCurrentPrice != NULL)
    {
        axapi::MarketDataField t_objCurrentPrice;
        memcpy_s(&t_objCurrentPrice, sizeof(t_objCurrentPrice), m_pCurrentPrice, sizeof(t_objCurrentPrice));

        double t_dbMA = t_objCurrentPrice.Volume == 0 ? 0 : (t_objCurrentPrice.Turnover / t_objCurrentPrice.Volume / m_objInstrumentInfo.VolumeMultiple);
        /// 触碰均线平仓
        if ((in_objHoldTrade.HighestProfitPrice - t_dbMA)
            * (t_objCurrentPrice.LastPrice - t_dbMA) < 0)
        {
            *ot_strOffsetMsg = (in_objHoldTrade.Direction == THOST_FTDC_D_Buy ? "bytchma" : "sltchma");
            *ot_blOffsetFlag = true;
        }
    }
}

void strategy_MA::getPreOffsetPrice(axapi::ConfirmedHoldTrade in_objHoldTrade, bool *ot_blSPOffsetFlag, double *ot_dbSPOffsetPrice)
{
    char* t_strLogFuncName = "strategy_MA::getPreOffsetPrice";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);

    *ot_blSPOffsetFlag = false;

    if (m_nProfitPoint2Offset > 0)
    {
        *ot_dbSPOffsetPrice = (in_objHoldTrade.Direction == THOST_FTDC_D_Buy) ? (in_objHoldTrade.Price + m_objInstrumentInfo.PriceTick * m_nProfitPoint2Offset) : (in_objHoldTrade.Price - m_objInstrumentInfo.PriceTick * m_nProfitPoint2Offset);
        *ot_blSPOffsetFlag = true;
    }
}