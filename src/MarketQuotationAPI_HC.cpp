#define MarketQuotationAPI_HC_EXE
#define DBDataSource
#define OTL_ORA10G
#define LOGGER_NAME "MarketQuotationAPI_HC"
#include "MarketQuotationAPI_HC.h"

#include <iostream>

int axapi::MarketQuotationAPI_HC::initializeLog()
{
    char *t_strLogFuncName = "axapi::MarketQuotationAPI_HC::initializeLog";
    log4cplus::initialize();
    log4cplus::helpers::LogLog::getLogLog()->setInternalDebugging(true);
    m_root = log4cplus::Logger::getRoot();
    m_objLogger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT(LOGGER_NAME));
    try {
        log4cplus::ConfigureAndWatchThread configureThread(
            LOG4CPLUS_TEXT("log4cplus.properties"), 5 * 1000);
    }
    catch (std::exception e) {
        LOG4CPLUS_FATAL(m_root, "initialLog exception");
    }
    return 0;
}

axapi::MarketQuotationAPI_HC::MarketQuotationAPI_HC(void)
{
    if (initializeLog() == 0)
    {
        //log4cplus_fatal(m_root, "initialize log ok");
        LOG4CPLUS_FATAL(m_objLogger, "initialize log ok");
    }
    else
    {
        LOG4CPLUS_FATAL(m_objLogger, "initialize log error");
    }
    try
    {
        /// 确认数据可加载
        otl_connect::otl_initialize();
        m_DBConnector.rlogon("ctp_marketdata/ctp_marketdata@ctp_mq");
        m_DBConnector.logoff();
        m_DBConnector.rlogon("marketdata/marketdata@center_data");
        m_DBConnector.logoff();
    }
    catch (otl_exception e)
    {
        LOG4CPLUS_FATAL(m_objLogger, e.msg);
    }
    m_pClockSimulated = NULL;
    loadTimeline();
}

axapi::MarketQuotationAPI_HC::~MarketQuotationAPI_HC(void)
{
}

int axapi::MarketQuotationAPI_HC::loadMarketData(const char *pInstrumentid, const char *pTradingday)
{
    char *t_strLogFuncName = "axapi::MarketQuotationAPI_HC::loadMarketData";
    char t_strLog[500];
    sprintf_s(t_strLog, sizeof(t_strLog), "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    MarketDataField t_objMarketData;

    /// 从数据存储中读取行情数据
    try
    {
        m_DBConnector.rlogon("ctp_marketdata/ctp_marketdata@ctp_mq");
        if (m_DBConnector.connected == 1)
        {
            /// m_DBConnector.server_attach();
            otl_stream marketdata(50, "select busiday, updatetime, substr(updatemillisec,1,1)+0, volume+0, turnover+0, openprice+0,"
                "  highestprice+0, lowestprice+0, lastprice+0, upperlimitprice+0,"
                "  lowerlimitprice+0, averageprice+0, bidprice1+0, bidvolume1+0, askprice1+0,"
                "  askvolume1+0 from ctp_marketdata.marketdata_history t"
                " where tradingday = :tradedate<char[9]>"
                "   and instrumentid = :instrument<char[10]>"
                "   and to_timestamp(busiday || ' ' || updatetime || '.' || rpad(to_char(updatemillisec), 1, '0'), 'yyyymmdd hh24:mi:ss.ff1')"
                "       >= to_timestamp('20190903 21:00:00', 'yyyymmdd hh24:mi:ss')"
                "   and to_timestamp(busiday || ' ' || updatetime || '.' || rpad(to_char(updatemillisec), 1, '0'), 'yyyymmdd hh24:mi:ss.ff1')"
                "       <= to_timestamp('20190904 15:15:00', 'yyyymmdd hh24:mi:ss')"
                " order by busiday, updatetime, updatemillisec",
                m_DBConnector);
            marketdata << pTradingday << pInstrumentid;
            int t_nClockCount = 0;
            while (!marketdata.eof())
            {
                int t_size = m_vMarketData.size();
                // 调试语句：行情加载不一致
                //if (t_size != t_nClockCount/* || t_nClockCount >= 84400*/)
                //{
                //    std::cout << "123" << std::endl;
                //}
                marketdata >> t_objMarketData.ActionDay >> t_objMarketData.UpdateTime >> t_objMarketData.UpdateMillisec >> t_objMarketData.Volume
                    >> t_objMarketData.Turnover >> t_objMarketData.OpenPrice >> t_objMarketData.HighestPrice >> t_objMarketData.LowestPrice
                    >> t_objMarketData.LastPrice >> t_objMarketData.UpperLimitPrice >> t_objMarketData.LowerLimitPrice >> t_objMarketData.AveragePrice
                    >> t_objMarketData.BidPrice1 >> t_objMarketData.BidVolume1 >> t_objMarketData.AskPrice1 >> t_objMarketData.AskVolume1;
                for (int i = t_nClockCount; i < m_vTradingClock.size(); i++)
                {
                    TradingClockField t_tmp = m_vTradingClock[i];

                    // 最新取到的行情是否与前一条处理的行情时间一致，如果一致则更新数据
                    if (i > 0)
                    {
                        if (strcmp(t_objMarketData.ActionDay, m_vTradingClock[i - 1].basedate) == 0
                            && strcmp(t_objMarketData.UpdateTime, m_vTradingClock[i - 1].minute) == 0
                            && t_objMarketData.UpdateMillisec == m_vTradingClock[i - 1].millisecond)
                        {
                            m_vMarketData.pop_back();
                            m_vMarketData.push_back(t_objMarketData);
                            break;
                        }
                    }

                    // 最新取到的行情为一下时间的行情
                    if (strcmp(t_objMarketData.ActionDay, t_tmp.basedate) == 0
                        && strcmp(t_objMarketData.UpdateTime, t_tmp.minute) == 0
                        && t_objMarketData.UpdateMillisec == t_tmp.millisecond)
                    {
                        m_vMarketData.push_back(t_objMarketData);
                        t_nClockCount = ++i;
                        break;
                    }
                    else
                    {
                        if (m_vMarketData.size() > 0)
                        {
                            m_vMarketData.push_back(m_vMarketData[m_vMarketData.size() - 1]);
                        }
                        else
                        {
                            m_vMarketData.push_back(t_objMarketData);
                        }
                        continue;
                    }
                }
            }
            for (int i = m_vMarketData.size(); i < m_vTradingClock.size(); i++)
            {
                m_vMarketData.push_back(m_vMarketData[i - 1]);
            }

            /// m_DBConnector.server_detach();
            m_DBConnector.logoff();
            sprintf_s(t_strLog, sizeof(t_strLog), "%s:Load MQ %d lines.", t_strLogFuncName, m_vMarketData.size());
            LOG4CPLUS_INFO(m_objLogger, t_strLog);
        }
        else
        {
            m_DBConnector.logoff();
            return -100;
        }
    }
    catch (otl_exception e)
    {
        m_DBConnector.logoff();
        sprintf_s(t_strLog, sizeof(t_strLog), "%s:<DB Error>.", t_strLogFuncName, e.msg);
        LOG4CPLUS_FATAL(m_objLogger, t_strLog);
        return -200;
    }
    catch (std::exception e)
    {
        m_DBConnector.logoff();
        sprintf_s(t_strLog, sizeof(t_strLog), "%s:<Error>.", t_strLogFuncName, e.what());
        LOG4CPLUS_FATAL(m_objLogger, t_strLog);
        return -300;
    }
    struct MarketDataField t_MarketDataField;
    memset(&t_MarketDataField, '\0', sizeof(MarketDataField));
    strcpy_s(t_MarketDataField.InstrumentID, sizeof(t_MarketDataField.InstrumentID), pInstrumentid);
    m_hashMarketDataList[pInstrumentid] = t_MarketDataField;
    return 0;
}

int axapi::MarketQuotationAPI_HC::loadTimeline()
{
    char *t_strLogFuncName = "axapi::MarketQuotationAPI_HC::loadTimeline";
    char t_strLog[500];
    sprintf_s(t_strLog, sizeof(t_strLog), "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    TradingClockField m_objTradingClock;

    /// 从数据存储中读取时间数据
    try
    {
        m_DBConnector.rlogon("marketdata/marketdata@center_data");
        if (m_DBConnector.connected == 1)
        {
            otl_stream marketdata(50, "select basedate, minute, millisecond, sequence, istradeflag from marketdata.vw_clock_byhalfsecond where 1=:f<int> order by sequence",
                m_DBConnector);
            marketdata << 1;
            while (!marketdata.eof())
            {
                marketdata >> m_objTradingClock.basedate >> m_objTradingClock.minute >> m_objTradingClock.millisecond
                    >> m_objTradingClock.sequence >> m_objTradingClock.istradeflag;
                m_vTradingClock.push_back(m_objTradingClock);
            }
            sprintf_s(t_strLog, sizeof(t_strLog), "%s:Load ClockData %d lines.", t_strLogFuncName, m_vTradingClock.size());
            LOG4CPLUS_INFO(m_objLogger, t_strLog);
            m_DBConnector.logoff();
        }
        else
        {
            m_DBConnector.logoff();
            return -100;
        }
    }
    catch (otl_exception e)
    {
        m_DBConnector.logoff();
        sprintf_s(t_strLog, sizeof(t_strLog), "%s:<DB Error>.", t_strLogFuncName, e.msg);
        LOG4CPLUS_FATAL(m_objLogger, t_strLog);
        return -200;
    }
    catch (std::exception e)
    {
        m_DBConnector.logoff();
        sprintf_s(t_strLog, sizeof(t_strLog), "%s:<Error>.", t_strLogFuncName, e.what());
        LOG4CPLUS_FATAL(m_objLogger, t_strLog);
        return -300;
    }

    return 0;
}

int axapi::MarketQuotationAPI_HC::initialClockSimulated(ClockSimulated *in_pClockSimulated)
{
    char *t_strLogFuncName = "axapi::MarketQuotationAPI_HC::initialClockSimulated";
    char t_strLog[500];
    sprintf_s(t_strLog, sizeof(t_strLog), "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    m_pClockSimulated = in_pClockSimulated;
    in_pClockSimulated->fillupClockLine(&m_vTradingClock);
    return 0;
}

void axapi::MarketQuotationAPI_HC::subMarketData(char *pInstrumentList[], int in_nInstrumentCount, char *pTradingday)
{
    char *t_strLogFuncName = "axapi::MarketQuotationAPI_HC::subMarketData";
    char t_strLog[500];
    sprintf_s(t_strLog, sizeof(t_strLog), "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);
}

int axapi::MarketQuotationAPI_HC::subMarketDataSingle(char *in_strInstrument, char *pTradingday)
{
    char *t_strLogFuncName = "axapi::MarketQuotationAPI_HC::subMarketDataSingle";
    char t_strLog[500];
    sprintf_s(t_strLog, sizeof(t_strLog), "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    /// 如果已经订阅过，则不重复订阅
    if (m_hashMarketDataList.find(in_strInstrument) == m_hashMarketDataList.end())
    {
#ifdef DBDataSource
        /// 根据需要从数据库中读取数据
        /// 打开数据库,加载数据
        if (loadMarketData(in_strInstrument, pTradingday) == 0)
#endif DBDataSource
        {
            sprintf_s(t_strLog, sizeof(t_strLog), "%s:Load %d MQData lines.", t_strLogFuncName, m_vMarketData.size());
            LOG4CPLUS_INFO(m_objLogger, t_strLog);
            return 0;
        }
        else
        {
            return -100;
        }
    }
    else
    {
        sprintf_s(t_strLog, sizeof(t_strLog), "%s:MQData has been loaded.", t_strLogFuncName);
        LOG4CPLUS_WARN(m_objLogger, t_strLog);
        return 0;
    }
}

axapi::MarketDataField *axapi::MarketQuotationAPI_HC::getCurrentPrice(const char *in_strInstrument)
{
#ifndef _FASTEST_MODE_
    char *t_strLogFuncName = "axapi::MarketQuotationAPI_HC::getCurrentPrice";
    char t_strLog[500];
    sprintf_s(t_strLog, sizeof(t_strLog), "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);
#endif

    return m_pClockSimulated == NULL ? NULL : &m_vMarketData[m_pClockSimulated->getCurrentTime()->sequence - 1];
}

axapi::MarketDataField *axapi::MarketQuotationAPI_HC::getCurrentPrice(const char *in_strInstrument, int *in_pClockSequence)
{
#ifndef _FASTEST_MODE_
    char *t_strLogFuncName = "axapi::MarketQuotationAPI_HC::getCurrentPrice";
    char t_strLog[500];
    sprintf_s(t_strLog, sizeof(t_strLog), "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);
#endif

    int seq = m_pClockSimulated->getCurrentTime()->sequence;
    *in_pClockSequence = seq;
    return m_pClockSimulated == NULL ? NULL : &m_vMarketData[seq - 1];
}
