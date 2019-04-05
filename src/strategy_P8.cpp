#define CTP_TRADEAPI
//#define KSV6T_TRADEAPI
#include "strategy_P8.h"

strategy_P8::strategy_P8(char * in_chInstrument, char * in_chPPPDirection, char * in_chPPNDirection, char * in_chPNPDirection, char * in_chPNNDirection, char * in_chNPPDirection, char * in_chNPNDirection, char * in_chNNPDirection, char * in_chNNNDirection, int in_nOffsetInterval, int in_nOffsetPriceDiff, int in_nOffsetPriceDiff2, int in_nProfitFallOffsetValve, double in_dbProfitFallRate)
{
    memset(&m_objFormerPrice1, '\0', sizeof(m_objFormerPrice1));
    memset(&m_objFormerPrice2, '\0', sizeof(m_objFormerPrice2));
    memset(&m_objInstrumentInfo, '\0', sizeof(m_objInstrumentInfo));
    m_pCurrentPrice = NULL;
    strcpy_s(m_chInstrument, sizeof(m_chInstrument), in_chInstrument);
    strcpy_s(m_chStrategy_PPP, sizeof(m_chStrategy_PPP), in_chPPPDirection);
    strcpy_s(m_chStrategy_PPN, sizeof(m_chStrategy_PPN), in_chPPNDirection);
    strcpy_s(m_chStrategy_PNP, sizeof(m_chStrategy_PNP), in_chPNPDirection);
    strcpy_s(m_chStrategy_PNN, sizeof(m_chStrategy_PNN), in_chPNNDirection);
    strcpy_s(m_chStrategy_NPP, sizeof(m_chStrategy_NPP), in_chNPPDirection);
    strcpy_s(m_chStrategy_NPN, sizeof(m_chStrategy_NPN), in_chNPNDirection);
    strcpy_s(m_chStrategy_NNP, sizeof(m_chStrategy_NNP), in_chNNPDirection);
    strcpy_s(m_chStrategy_NNN, sizeof(m_chStrategy_NNN), in_chNNNDirection);
    m_nProfitFallOffsetValve = in_nProfitFallOffsetValve;
    m_nOffsetPriceDiff = in_nOffsetPriceDiff;
    m_nOffsetPriceDiff2 = in_nOffsetPriceDiff2;
    m_nOffsetInterval = in_nOffsetInterval;
    m_dbProfitFallRate = in_dbProfitFallRate;
    while (m_objInstrumentInfo.InstrumentID[0] == '\0')
    {
        m_objInstrumentInfo = m_pTrade->getInstrumentInfo(m_chInstrument);
    }
    while (m_pCurrentPrice == NULL)
    {
        m_pCurrentPrice = m_pMarketQuotation->getCurrentPrice(m_chInstrument);
    }
}


strategy_P8::strategy_P8(char * in_chInstrument, char * in_chPPPDirection, char * in_chPPNDirection, char * in_chPNPDirection, char * in_chPNNDirection, char * in_chNPPDirection, char * in_chNPNDirection, char * in_chNNPDirection, char * in_chNNNDirection, int in_nOffsetInterval, int in_nOffsetPriceDiff, int in_nOffsetPriceDiff2, int in_nProfitFallOffsetValve, double in_dbProfitFallRate)
{
}

strategy_P8::~strategy_P8()
{
}

int strategy_P8::initializeSubLog()
{
    m_objLoggerSub = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("strategy_P8"));
    return 0;
}

void strategy_P8::myStrategy(bool *ot_blOpenFlag, std::string *ot_strOpenMsg, char *ot_strContract, int *ot_nDirection, int *ot_nOffsetFlag, int *ot_nOrderTypeFlag, int *ot_nOrderAmount, double *ot_dOrderPrice)
{
    char* t_strLogFuncName = "strategy_P8::myStrategy";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);

    double t_PriceChange, t_VolumeChange, t_OpenInterestChange;
    *ot_blOpenFlag = false;

    if (m_pCurrentPrice != NULL)
    {
        axapi::MarketDataField t_objCurrentPrice;
        memcpy_s(&t_objCurrentPrice, sizeof(t_objCurrentPrice), m_pCurrentPrice, sizeof(t_objCurrentPrice));
        if (m_objFormerPrice1.LastPrice == 0)
        {
            m_objFormerPrice1.LastPrice = t_objCurrentPrice.LastPrice;
            m_objFormerPrice1.Volume = t_objCurrentPrice.Volume;
            m_objFormerPrice1.OpenInterest = t_objCurrentPrice.OpenInterest;
            m_objFormerPrice2.LastPrice = t_objCurrentPrice.LastPrice;
            m_objFormerPrice2.Volume = t_objCurrentPrice.Volume;
            m_objFormerPrice2.OpenInterest = t_objCurrentPrice.OpenInterest;
            return;
        }

        // 策略主体，判断方向开仓
        t_PriceChange = t_objCurrentPrice.LastPrice - m_objFormerPrice2.LastPrice;
        t_VolumeChange = (t_objCurrentPrice.Volume - m_objFormerPrice2.Volume) - (m_objFormerPrice2.Volume - m_objFormerPrice1.Volume);
        t_OpenInterestChange = t_objCurrentPrice.OpenInterest - m_objFormerPrice2.OpenInterest;
        sprintf_s(t_strLog, "PriceChange:%f;VolumeChange:%f;OpenInterestChange:%f;AveragePrice:%f",
            t_PriceChange,
            t_VolumeChange,
            t_OpenInterestChange,
            (t_objCurrentPrice.Turnover / t_objCurrentPrice.Volume / m_objInstrumentInfo.VolumeMultiple));
        LOG4CPLUS_INFO(m_objLoggerSub, t_strLog);
        if (t_PriceChange > 0 && t_VolumeChange > 0 && t_OpenInterestChange > 0)
        {
            if (strcmp(m_chStrategy_PPP, "sell") == 0 && t_objCurrentPrice.LastPrice < (t_objCurrentPrice.Turnover / t_objCurrentPrice.Volume / m_objInstrumentInfo.VolumeMultiple))
            {
                *ot_blOpenFlag = true;
                *ot_strOpenMsg = "PPP sell open order";
                *ot_nDirection = ORDER_DIRECTION_SELL;
            }
            else if (strcmp(m_chStrategy_PPP, "buy") == 0 && t_objCurrentPrice.LastPrice > (t_objCurrentPrice.Turnover / t_objCurrentPrice.Volume / m_objInstrumentInfo.VolumeMultiple))
            {
                *ot_blOpenFlag = true;
                *ot_strOpenMsg = "PPP buy open order";
                *ot_nDirection = ORDER_DIRECTION_BUY;
            }
            else
            {
                *ot_blOpenFlag = false;
            }
        }
        else if (t_PriceChange > 0 && t_VolumeChange > 0 && t_OpenInterestChange < 0)
        {
            if (strcmp(m_chStrategy_PPN, "sell") == 0 && t_objCurrentPrice.LastPrice < (t_objCurrentPrice.Turnover / t_objCurrentPrice.Volume / m_objInstrumentInfo.VolumeMultiple))
            {
                *ot_blOpenFlag = true;
                *ot_strOpenMsg = "PPN sell open order";
                *ot_nDirection = ORDER_DIRECTION_SELL;
            }
            else if (strcmp(m_chStrategy_PPN, "buy") == 0 && t_objCurrentPrice.LastPrice > (t_objCurrentPrice.Turnover / t_objCurrentPrice.Volume / m_objInstrumentInfo.VolumeMultiple))
            {
                *ot_blOpenFlag = true;
                *ot_strOpenMsg = "PPN buy open order";
                *ot_nDirection = ORDER_DIRECTION_BUY;
            }
            else
            {
                *ot_blOpenFlag = false;
            }
        }
        else if (t_PriceChange > 0 && t_VolumeChange < 0 && t_OpenInterestChange > 0)
        {
            if (strcmp(m_chStrategy_PNP, "sell") == 0 && t_objCurrentPrice.LastPrice < (t_objCurrentPrice.Turnover / t_objCurrentPrice.Volume / m_objInstrumentInfo.VolumeMultiple))
            {
                *ot_blOpenFlag = true;
                *ot_strOpenMsg = "PNP sell open order";
                *ot_nDirection = ORDER_DIRECTION_SELL;
            }
            else if (strcmp(m_chStrategy_PNP, "buy") == 0 && t_objCurrentPrice.LastPrice > (t_objCurrentPrice.Turnover / t_objCurrentPrice.Volume / m_objInstrumentInfo.VolumeMultiple))
            {
                *ot_blOpenFlag = true;
                *ot_strOpenMsg = "PNP buy open order";
                *ot_nDirection = ORDER_DIRECTION_BUY;
            }
            else
            {
                *ot_blOpenFlag = false;
            }
        }
        else if (t_PriceChange > 0 && t_VolumeChange < 0 && t_OpenInterestChange < 0)
        {
            if (strcmp(m_chStrategy_PNN, "sell") == 0 && t_objCurrentPrice.LastPrice < (t_objCurrentPrice.Turnover / t_objCurrentPrice.Volume / m_objInstrumentInfo.VolumeMultiple))
            {
                *ot_blOpenFlag = true;
                *ot_strOpenMsg = "PNN sell open order";
                *ot_nDirection = ORDER_DIRECTION_SELL;
            }
            else if (strcmp(m_chStrategy_PNN, "buy") == 0 && t_objCurrentPrice.LastPrice > (t_objCurrentPrice.Turnover / t_objCurrentPrice.Volume / m_objInstrumentInfo.VolumeMultiple))
            {
                *ot_blOpenFlag = true;
                *ot_strOpenMsg = "PNN buy open order";
                *ot_nDirection = ORDER_DIRECTION_BUY;
            }
            else
            {
                *ot_blOpenFlag = false;
            }
        }
        else if (t_PriceChange < 0 && t_VolumeChange > 0 && t_OpenInterestChange > 0)
        {
            if (strcmp(m_chStrategy_NPP, "sell") == 0 && t_objCurrentPrice.LastPrice < (t_objCurrentPrice.Turnover / t_objCurrentPrice.Volume / m_objInstrumentInfo.VolumeMultiple))
            {
                *ot_blOpenFlag = true;
                *ot_strOpenMsg = "NPP sell open order";
                *ot_nDirection = ORDER_DIRECTION_SELL;
            }
            else if (strcmp(m_chStrategy_NPP, "buy") == 0 && t_objCurrentPrice.LastPrice > (t_objCurrentPrice.Turnover / t_objCurrentPrice.Volume / m_objInstrumentInfo.VolumeMultiple))
            {
                *ot_blOpenFlag = true;
                *ot_strOpenMsg = "NPP buy open order";
                *ot_nDirection = ORDER_DIRECTION_BUY;
            }
            else
            {
                *ot_blOpenFlag = false;
            }
        }
        else if (t_PriceChange < 0 && t_VolumeChange > 0 && t_OpenInterestChange < 0)
        {
            if (strcmp(m_chStrategy_NPN, "sell") == 0 && t_objCurrentPrice.LastPrice < (t_objCurrentPrice.Turnover / t_objCurrentPrice.Volume / m_objInstrumentInfo.VolumeMultiple))
            {
                *ot_blOpenFlag = true;
                *ot_strOpenMsg = "NPN sell open order";
                *ot_nDirection = ORDER_DIRECTION_SELL;
            }
            else if (strcmp(m_chStrategy_NPN, "buy") == 0 && t_objCurrentPrice.LastPrice > (t_objCurrentPrice.Turnover / t_objCurrentPrice.Volume / m_objInstrumentInfo.VolumeMultiple))
            {
                *ot_blOpenFlag = true;
                *ot_strOpenMsg = "NPN buy open order";
                *ot_nDirection = ORDER_DIRECTION_BUY;
            }
            else
            {
                *ot_blOpenFlag = false;
            }
        }
        else if (t_PriceChange < 0 && t_VolumeChange < 0 && t_OpenInterestChange > 0)
        {
            if (strcmp(m_chStrategy_NNP, "sell") == 0 && t_objCurrentPrice.LastPrice < (t_objCurrentPrice.Turnover / t_objCurrentPrice.Volume / m_objInstrumentInfo.VolumeMultiple))
            {
                *ot_blOpenFlag = true;
                *ot_strOpenMsg = "NNP sell open order";
                *ot_nDirection = ORDER_DIRECTION_SELL;
            }
            else if (strcmp(m_chStrategy_NNP, "buy") == 0 && t_objCurrentPrice.LastPrice > (t_objCurrentPrice.Turnover / t_objCurrentPrice.Volume / m_objInstrumentInfo.VolumeMultiple))
            {
                *ot_blOpenFlag = true;
                *ot_strOpenMsg = "NNP buy open order";
                *ot_nDirection = ORDER_DIRECTION_BUY;
            }
            else
            {
                *ot_blOpenFlag = false;
            }
        }
        else if (t_PriceChange < 0 && t_VolumeChange < 0 && t_OpenInterestChange < 0)
        {
            if (strcmp(m_chStrategy_NNN, "sell") == 0 && t_objCurrentPrice.LastPrice < (t_objCurrentPrice.Turnover / t_objCurrentPrice.Volume / m_objInstrumentInfo.VolumeMultiple))
            {
                *ot_blOpenFlag = true;
                *ot_strOpenMsg = "NNN sell open order";
                *ot_nDirection = ORDER_DIRECTION_SELL;
            }
            else if (strcmp(m_chStrategy_NNN, "buy") == 0 && t_objCurrentPrice.LastPrice > (t_objCurrentPrice.Turnover / t_objCurrentPrice.Volume / m_objInstrumentInfo.VolumeMultiple))
            {
                *ot_blOpenFlag = true;
                *ot_strOpenMsg = "NNN buy open order";
                *ot_nDirection = ORDER_DIRECTION_BUY;
            }
            else
            {
                *ot_blOpenFlag = false;
            }
        }

        m_objFormerPrice1.LastPrice = m_objFormerPrice2.LastPrice;
        m_objFormerPrice1.Volume = m_objFormerPrice2.Volume;
        m_objFormerPrice1.OpenInterest = m_objFormerPrice2.OpenInterest;
        m_objFormerPrice2.LastPrice = t_objCurrentPrice.LastPrice;
        m_objFormerPrice2.Volume = t_objCurrentPrice.Volume;
        m_objFormerPrice2.OpenInterest = t_objCurrentPrice.OpenInterest;

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

void strategy_P8::myOffsetStrategy(axapi::ConfirmedHoldTrade in_objHoldTrade, bool *ot_blOffsetFlag, std::string *ot_strOffsetMsg)
{
    char* t_strLogFuncName = "Strategy::myOffsetStrategy";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);

    *ot_blOffsetFlag = false;
    /*
    * 是否强平
    * 0.最大回撤
    * 1.n秒强平
    * 2.止损平仓
    * 3.止盈平仓
    */
    /*
    * 准备策略参数
    * 1.当前时间
    * 2.当前行情
    * 3.合约最小变动价位
    */
    /// 准备当前时间参数
    // 用于获取时间
    time_t nowtime;
    tm *curtime;
    int curMinutes, t_TradeInfoTime;
    char t_TradeInfoHour[3];
    char t_TradeInfoMinutes[3];
    char t_TradeInfoSeconds[3];

    nowtime = time(NULL);
    curtime = localtime(&nowtime);
    t_TradeInfoHour[0] = in_objHoldTrade.TradeTime[0];
    t_TradeInfoHour[1] = in_objHoldTrade.TradeTime[1];
    t_TradeInfoHour[2] = '\0';
    t_TradeInfoMinutes[0] = in_objHoldTrade.TradeTime[3];
    t_TradeInfoMinutes[1] = in_objHoldTrade.TradeTime[4];
    t_TradeInfoMinutes[2] = '\0';
    t_TradeInfoSeconds[0] = in_objHoldTrade.TradeTime[6];
    t_TradeInfoSeconds[1] = in_objHoldTrade.TradeTime[7];
    t_TradeInfoSeconds[2] = '\0';

    /// 处理跨日的问题
    curMinutes = (curtime->tm_hour + (curtime->tm_hour < 20 ? 24 : 0)) * 3600 + curtime->tm_min * 60 + curtime->tm_sec * 1;
    t_TradeInfoTime = (atoi(t_TradeInfoHour) + (atoi(t_TradeInfoHour) < 20 ? 24 : 0)) * 3600 + atoi(t_TradeInfoMinutes) * 60 + atoi(t_TradeInfoSeconds);

    sprintf_s(t_strLog, "%s:curMinutes:%d,|%d:%d:%d", t_strLogFuncName, curMinutes, curtime->tm_hour, curtime->tm_min, curtime->tm_sec);
    LOG4CPLUS_DEBUG(m_objLoggerSub, t_strLog);
    sprintf_s(t_strLog, "%s:t_TradeInfoHour:%d,|%s|%s:%s:%s", t_strLogFuncName, t_TradeInfoTime, in_objHoldTrade.TradeTime, t_TradeInfoHour, t_TradeInfoMinutes, t_TradeInfoSeconds);
    LOG4CPLUS_DEBUG(m_objLoggerSub, t_strLog);
    sprintf_s(t_strLog, "%s:curMinutes:%d,t_TradeInfoTime:%d", t_strLogFuncName, curMinutes, t_TradeInfoTime);
    LOG4CPLUS_DEBUG(m_objLoggerSub, t_strLog);

    if (m_pCurrentPrice != NULL)
    {
        /// 准备当前行情价
        axapi::MarketDataField t_objCurrentPrice;
        memcpy_s(&t_objCurrentPrice, sizeof(t_objCurrentPrice), m_pCurrentPrice, sizeof(t_objCurrentPrice));

        // 达到盈利阀值最大回撤比例止盈
        if ((in_objHoldTrade.Direction == THOST_FTDC_D_Buy
            && (t_objCurrentPrice.LastPrice - in_objHoldTrade.HighestProfitPrice) >= m_nProfitFallOffsetValve * m_objInstrumentInfo.PriceTick)
            || (in_objHoldTrade.Direction == THOST_FTDC_D_Sell
                && (t_objCurrentPrice.LastPrice - in_objHoldTrade.HighestProfitPrice) * (-1) >= m_nProfitFallOffsetValve * m_objInstrumentInfo.PriceTick))
        {
            if ((in_objHoldTrade.Direction == THOST_FTDC_D_Buy
                && t_objCurrentPrice.LastPrice <= in_objHoldTrade.HighestProfitPrice - (in_objHoldTrade.HighestProfitPrice - in_objHoldTrade.Price) * m_dbProfitFallRate)
                || (in_objHoldTrade.Direction == THOST_FTDC_D_Sell
                    && t_objCurrentPrice.LastPrice >= in_objHoldTrade.HighestProfitPrice + (in_objHoldTrade.HighestProfitPrice - in_objHoldTrade.Price) * (-1) * m_dbProfitFallRate))
            {
                *ot_strOffsetMsg = "profitfall";
                *ot_blOffsetFlag = true;
            }
        }
        // 持仓时间超过限制则平仓
        else if (curMinutes - t_TradeInfoTime >= m_nOffsetInterval)
        {
            *ot_strOffsetMsg = "timeout";
            *ot_blOffsetFlag = true;
        }
        // 多单止损平仓
        else if (in_objHoldTrade.Direction == THOST_FTDC_D_Buy
            && (t_objCurrentPrice.LastPrice - in_objHoldTrade.HighestProfitPrice)
            <= (-1) * m_nOffsetPriceDiff * m_objInstrumentInfo.PriceTick)
        {
            *ot_strOffsetMsg = "Buy_lossover";
            *ot_blOffsetFlag = true;
        }
        // 空单止损平仓
        else if (in_objHoldTrade.Direction == THOST_FTDC_D_Sell
            && (t_objCurrentPrice.LastPrice - in_objHoldTrade.HighestProfitPrice)
            >= m_nOffsetPriceDiff * m_objInstrumentInfo.PriceTick)
        {
            *ot_strOffsetMsg = "Sell_lossover";
            *ot_blOffsetFlag = true;
        }
        // 多单止盈平仓 g_nOffsetPriceDiff2=0 无止盈平仓动作
        else if (m_nOffsetPriceDiff2 > 0
            && in_objHoldTrade.Direction == THOST_FTDC_D_Buy
            && (t_objCurrentPrice.LastPrice - in_objHoldTrade.Price)
            >= m_nOffsetPriceDiff2 * m_objInstrumentInfo.PriceTick)
        {
            *ot_strOffsetMsg = "Buy_profitover";
            *ot_blOffsetFlag = true;
        }
        // 空单止盈平仓 g_nOffsetPriceDiff2=0 无止盈平仓动作
        else if (m_nOffsetPriceDiff2 > 0
            && in_objHoldTrade.Direction == THOST_FTDC_D_Sell
            && (t_objCurrentPrice.LastPrice - in_objHoldTrade.Price)
            <= (-1) * m_nOffsetPriceDiff2 * m_objInstrumentInfo.PriceTick)
        {
            *ot_strOffsetMsg = "Sell_profitover";
            *ot_blOffsetFlag = true;
        }
    }
}

void strategy_P8::getPreOffsetPrice(axapi::ConfirmedHoldTrade in_objHoldTrade, bool *ot_blSPOffsetFlag, double *ot_dbSPOffsetPrice)
{
    char* t_strLogFuncName = "Strategy::getPreOffsetPrice";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);

    *ot_blSPOffsetFlag = false;

}