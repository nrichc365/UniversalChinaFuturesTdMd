#define STRATEGY_EXP
#define CTP_TRADEAPI
//#define KSV6T_TRADEAPI
#define LOGGER_NAME "Strategy"
#include "Strategy.h"
#include <TradeAPITypeDefine.h>
#include <iostream>
#include <thread>
#include <DataIntoFiles\DataIntoFiles.h>

#define UnpairedOffsetOrderCheckSequenceDiff 200  ///当UnpairedOffsetOrder中的记录被删除前，需要经过的最小的确认次数(updateorderinfo执行的次数差)

int axapi::Strategy::initializeLog()
{
    char *t_strLogFuncName = "axapi::Strategy::initializeLog";
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

int axapi::Strategy::initializeAPI(axapi::MarketQuotationAPI *in_pMarketQuotation, axapi::TradeAPI *in_pTrade)
{
    return setAPI(in_pMarketQuotation, in_pTrade);
}

int axapi::Strategy::setAPI(axapi::MarketQuotationAPI *in_pMarketQuotation, axapi::TradeAPI *in_pTrade)
{
    char *t_strLogFuncName = "Strategy::setAPI";
    char t_strLog[500];
    sprintf_s(t_strLog, sizeof(t_strLog), "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    if (in_pMarketQuotation == NULL || in_pTrade == NULL)
    {
        sprintf_s(t_strLog, sizeof(t_strLog), "%s:TdAPI&MdAPI is NULL", t_strLogFuncName);
        LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
        return -100;
    }
    else
    {
        m_pTrade = in_pTrade;
        m_pMarketQuotation = in_pMarketQuotation;
    }
    return 0;
}

axapi::Strategy::Strategy(void)
{
    if (initializeLog() == 0)
    {
        LOG4CPLUS_FATAL(m_objLogger, "initialize LOG OK");
    }
    else
    {
        LOG4CPLUS_FATAL(m_objLogger, "initialize LOG ERROR");
    }
    m_pMarketQuotation = NULL;
    m_pTrade = NULL;

    m_blOffsetALLRunning = false;
    m_blOffsetRunning = false;
    m_blOpenRunning = false;
    m_blCancelRunning = false;
    m_blShutdownFlag = false;
    m_hOpenFinished = CreateEvent(NULL, true, false, NULL);;
    m_hOffsetFinished = CreateEvent(NULL, true, false, NULL);
    m_hCancelFinished = CreateEvent(NULL, true, false, NULL);
    m_hOffsetAllFinished = CreateEvent(NULL, true, false, NULL);
    m_hUpdateOrderFinished = CreateEvent(NULL, true, false, NULL);
    m_hUpdateTradeFinished = CreateEvent(NULL, true, false, NULL);

    m_nCancelWaitSeconds = 20;
    m_nOrderingCheckWaitMillseconds = 1000;
    m_nUpdateOrderTimes = 0;
    m_nUpdateTradeTimes = 0;
#ifdef STRATEGY_EXE
    m_nOpenCount = 0;
#endif STRATEGY_EXE
}

axapi::Strategy::~Strategy(void)
{
    char *t_strLogFuncName = "Strategy::~Strategy";
    char t_strLog[500];
    sprintf_s(t_strLog, sizeof(t_strLog), "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    stop();
    m_hashAllOrder.clear();
    m_hashConfirmedHoldTrade.clear();
    m_hashConfirmedOrder.clear();
    m_hashUnpairedOffsetOrder.clear();
    m_vConfirmedHoldTrade.clear();
    m_vConfirmedOrder.clear();
}

void axapi::Strategy::saveData()
{
    /*
    * 将当前运行数据存入数据文件,数据文件以时间命名
    */
    char *t_strLogFuncName = "Strategy::saveData";
    char t_strLog[500];
    sprintf_s(t_strLog, sizeof(t_strLog), "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    time_t nowtime;
    tm *curtime;
    nowtime = time(NULL);
    curtime = localtime(&nowtime);
    char t_datafile[50];
    memset(&t_datafile, '\0', sizeof(t_datafile));
    sprintf_s(t_datafile, sizeof(t_datafile), "datafield", curtime->tm_year, curtime->tm_mon + 1, curtime->tm_mday, curtime->tm_hour, curtime->tm_min, curtime->tm_sec);

    /*
    * 写入文件
    */
    DataIntoFiles *t_pDataIntoFiles = new DataIntoFiles(t_datafile, 50, "datafile.list");
    /// 记录m_hashAllOrder
    for (std::hash_map<std::string/*UniversalChinaFutureTdOrderRefType*/, struct AllOrder>::iterator i = m_hashAllOrder.begin();
        i != m_hashAllOrder.end(); i++)
    {
        sprintf_s(t_strLog, sizeof(t_strLog), "m_hashAllOrder[%s]:OrderID(%s),OrderRef(%s),OrderType(%c),HoldTradeID(%s),updateOrderRoundSequence(%d)", i->first.c_str(), i->second.OrderID, i->second.OrderRef, i->second.OrderType, i->second.HoldTradeID, i->second.updateOrderRoundSequence);
        t_pDataIntoFiles->writeData2File(t_strLog);
    }
    /// 记录m_hashConfirmedOrder
    for (std::hash_map<std::string/*UniversalChinaFutureTdOrderIDType*/, UniversalChinaFutureTdSequenceType>::iterator i = m_hashConfirmedOrder.begin();
        i != m_hashConfirmedOrder.end(); i++)
    {
        sprintf_s(t_strLog, sizeof(t_strLog), "m_hashConfirmedOrder[%s]:Sequence(%d)", i->first.c_str(), i->second);
        t_pDataIntoFiles->writeData2File(t_strLog);
    }
    /// 记录m_vConfirmedOrder
    for (int i = 0; i < m_vConfirmedOrder.size(); i++)
    {
        sprintf_s(t_strLog, sizeof(t_strLog), "m_vConfirmedOrder[%d]:OrderID(%s),HoldTradeID(%s),OrderRef(%s),InsertTime(%s),\
                            OrderStatus(%c),OrderType(%c),OrderPrice(%lf),OrderVolumeTotal(%d),\
                            OrderVolumeTraded(%d),OrderVolumnOriginal(%d),TradeIDList.size(%d)",
            i, m_vConfirmedOrder[i].OrderID, m_vConfirmedOrder[i].HoldTradeID, m_vConfirmedOrder[i].OrderRef, m_vConfirmedOrder[i].InsertTime,
            m_vConfirmedOrder[i].OrderStatus, m_vConfirmedOrder[i].OrderType, m_vConfirmedOrder[i].OrderPrice, m_vConfirmedOrder[i].OrderVolumeTotal,
            m_vConfirmedOrder[i].OrderVolumeTraded, m_vConfirmedOrder[i].OrderVolumnOriginal, m_vConfirmedOrder[i].TradeIDList.size());
        t_pDataIntoFiles->writeData2File(t_strLog);
    }
    /// 记录m_hashConfirmedHoldTrade
    for (std::hash_map<std::string/*UniversalChinaFutureTdTradeIDType*/, UniversalChinaFutureTdSequenceType>::iterator i = m_hashConfirmedHoldTrade.begin();
        i != m_hashConfirmedHoldTrade.end(); i++)
    {
        sprintf_s(t_strLog, sizeof(t_strLog), "m_hashConfirmedHoldTrade[%s]:Sequence(%d)", i->first.c_str(), i->second);
        t_pDataIntoFiles->writeData2File(t_strLog);
    }
    /// 记录m_vConfirmedHoldTrade
    for (int i = 0; i < m_vConfirmedHoldTrade.size(); i++)
    {
        sprintf_s(t_strLog, sizeof(t_strLog), "m_vConfirmedHoldTrade[%d]:TradeID(%s),TradeStatus(%d),InstrumentID(%s),Direction(%d),\
                            Price(%lf),Volumn(%d),OffsetVolumn(%d),AvailableVolumn(%d),\
                            SPOrderID(%s),SLOrderID.size(%d)",
            i, m_vConfirmedHoldTrade[i].TradeID, m_vConfirmedHoldTrade[i].TradeStatus, m_vConfirmedHoldTrade[i].InstrumentID, m_vConfirmedHoldTrade[i].Direction,
            m_vConfirmedHoldTrade[i].Price, m_vConfirmedHoldTrade[i].Volumn, m_vConfirmedHoldTrade[i].OffsetVolumn, m_vConfirmedHoldTrade[i].AvailableVolumn,
            m_vConfirmedHoldTrade[i].SPOrderID, m_vConfirmedHoldTrade[i].SLOrderID.size());
        t_pDataIntoFiles->writeData2File(t_strLog);
    }
    /// 记录m_hashUnpairedOffsetOrder
    for (std::hash_map<std::string/*UniversalChinaFutureTdTradeIDType*/, struct UnpairedOffsetOrder>::iterator i = m_hashUnpairedOffsetOrder.begin();
        i != m_hashUnpairedOffsetOrder.end(); i++)
    {
        sprintf_s(t_strLog, sizeof(t_strLog), "m_hashUnpairedOffsetOrder[%s]:TradeID(%s),OrderRef(%s),OffsetOrderType(%c)", i->first.c_str(), i->second.TradeID, i->second.OrderRef, i->second.OffsetOrderType);
        t_pDataIntoFiles->writeData2File(t_strLog);
    }
    /// 记录参数
    sprintf_s(t_strLog, sizeof(t_strLog), "m_nUpdateOrderTimes:%d", m_nUpdateOrderTimes);
    t_pDataIntoFiles->writeData2File(t_strLog);
    sprintf_s(t_strLog, sizeof(t_strLog), "m_nUpdateTradeTimes:%d", m_nUpdateTradeTimes);
    t_pDataIntoFiles->writeData2File(t_strLog);
    sprintf_s(t_strLog, sizeof(t_strLog), "m_nOrderingCheckWaitMillseconds:%d", m_nOrderingCheckWaitMillseconds);
    t_pDataIntoFiles->writeData2File(t_strLog);
    sprintf_s(t_strLog, sizeof(t_strLog), "m_nCancelWaitSeconds:%d", m_nCancelWaitSeconds);
    t_pDataIntoFiles->writeData2File(t_strLog);
}

void axapi::Strategy::loadData()
{
    char *t_strLogFuncName = "Strategy::loadData";
    char t_strLog[500];
    sprintf_s(t_strLog, sizeof(t_strLog), "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    /*AllOrder t_objAllOrder;
    strcpy_s(t_objAllOrder.OrderID, sizeof(t_objAllOrder.OrderID), "    40000477");
    strcpy_s(t_objAllOrder.OrderRef, sizeof(t_objAllOrder.OrderRef), "34");
    t_objAllOrder.OrderType = OrderStatus_Open;
    t_objAllOrder.updateOrderRoundSequence = 1;
    memset(t_objAllOrder.HoldTradeID, '\0', sizeof(t_objAllOrder.HoldTradeID));
    m_hashAllOrder["34"] = t_objAllOrder;

    strcpy_s(t_objAllOrder.OrderID, sizeof(t_objAllOrder.OrderID), "    40000478");
    strcpy_s(t_objAllOrder.OrderRef, sizeof(t_objAllOrder.OrderRef), "35");
    t_objAllOrder.OrderType = OrderStatus_Open;
    t_objAllOrder.updateOrderRoundSequence = 1;
    memset(t_objAllOrder.HoldTradeID, '\0', sizeof(t_objAllOrder.HoldTradeID));
    m_hashAllOrder["35"] = t_objAllOrder;

    strcpy_s(t_objAllOrder.OrderID, sizeof(t_objAllOrder.OrderID), "    40000480");
    strcpy_s(t_objAllOrder.OrderRef, sizeof(t_objAllOrder.OrderRef), "36");
    t_objAllOrder.OrderType = OrderStatus_Open;
    t_objAllOrder.updateOrderRoundSequence = 1;
    memset(t_objAllOrder.HoldTradeID, '\0', sizeof(t_objAllOrder.HoldTradeID));
    m_hashAllOrder["36"] = t_objAllOrder;

    ConfirmedOrder t_objConfirmedOrder;
    memset(t_objConfirmedOrder.HoldTradeID, '\0', sizeof(t_objConfirmedOrder.HoldTradeID));
    strcpy_s(t_objConfirmedOrder.InsertTime, sizeof(t_objConfirmedOrder.InsertTime), "13:50:08");
    strcpy_s(t_objConfirmedOrder.OrderID, sizeof(t_objConfirmedOrder.OrderID), "    40000477");
    strcpy_s(t_objConfirmedOrder.OrderRef, sizeof(t_objConfirmedOrder.OrderRef), "34");
    t_objConfirmedOrder.OrderPrice = 702.5;
    t_objConfirmedOrder.OrderStatus = THOST_FTDC_OST_AllTraded;
    t_objConfirmedOrder.OrderType = OrderStatus_Open;
    t_objConfirmedOrder.OrderVolumeTotal = 0;
    t_objConfirmedOrder.OrderVolumeTraded = 1;
    t_objConfirmedOrder.OrderVolumnOriginal = 1;
    t_objConfirmedOrder.TradeIDList.clear();
    m_vConfirmedOrder.push_back(t_objConfirmedOrder);
    m_hashConfirmedOrder["    40000477"] = 0;

    memset(t_objConfirmedOrder.HoldTradeID, '\0', sizeof(t_objConfirmedOrder.HoldTradeID));
    strcpy_s(t_objConfirmedOrder.InsertTime, sizeof(t_objConfirmedOrder.InsertTime), "13:50:09");
    strcpy_s(t_objConfirmedOrder.OrderID, sizeof(t_objConfirmedOrder.OrderID), "    40000478");
    strcpy_s(t_objConfirmedOrder.OrderRef, sizeof(t_objConfirmedOrder.OrderRef), "35");
    t_objConfirmedOrder.OrderPrice = 702.5;
    t_objConfirmedOrder.OrderStatus = THOST_FTDC_OST_AllTraded;
    t_objConfirmedOrder.OrderType = OrderStatus_Open;
    t_objConfirmedOrder.OrderVolumeTotal = 0;
    t_objConfirmedOrder.OrderVolumeTraded = 1;
    t_objConfirmedOrder.OrderVolumnOriginal = 1;
    t_objConfirmedOrder.TradeIDList.clear();
    m_vConfirmedOrder.push_back(t_objConfirmedOrder);
    m_hashConfirmedOrder["    40000478"] = 1;

    memset(t_objConfirmedOrder.HoldTradeID, '\0', sizeof(t_objConfirmedOrder.HoldTradeID));
    strcpy_s(t_objConfirmedOrder.InsertTime, sizeof(t_objConfirmedOrder.InsertTime), "13:50:10");
    strcpy_s(t_objConfirmedOrder.OrderID, sizeof(t_objConfirmedOrder.OrderID), "    40000480");
    strcpy_s(t_objConfirmedOrder.OrderRef, sizeof(t_objConfirmedOrder.OrderRef), "36");
    t_objConfirmedOrder.OrderPrice = 702.5;
    t_objConfirmedOrder.OrderStatus = THOST_FTDC_OST_AllTraded;
    t_objConfirmedOrder.OrderType = OrderStatus_Open;
    t_objConfirmedOrder.OrderVolumeTotal = 0;
    t_objConfirmedOrder.OrderVolumeTraded = 1;
    t_objConfirmedOrder.OrderVolumnOriginal = 1;
    t_objConfirmedOrder.TradeIDList.clear();
    m_vConfirmedOrder.push_back(t_objConfirmedOrder);
    m_hashConfirmedOrder["    40000480"] = 2;*/
}

void axapi::Strategy::startStrategy()
{
    char *t_strLogFuncName = "Strategy::startStrategy";
    char t_strLog[500];
    sprintf_s(t_strLog, sizeof(t_strLog), "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    /*
    * 防止多次运行的判别
    */
    if (m_blStrategyRunning)
    {
        return;
    }

    /// 开进程运行策略
    loadData();
    m_blShutdownFlag = false;
    m_blOffsetALLRunning = false;
    m_blCancelRunning = true;
    m_blOffsetRunning = true;
    m_blOpenRunning = true;
    m_blStrategyRunning = true;
    std::thread autorun_updateOrderInfo(&Strategy::updateOrderInfo, this);
    std::thread autorun_updateTradeInfo(&Strategy::updateTradeInfo, this);
    std::thread autorun_strategyOrder(&Strategy::strategyOrder, this);
    std::thread autorun_strategyOffset(&Strategy::strategyOffset, this);
    std::thread autorun_strategyOffsetALL(&Strategy::strategyOffsetALL, this);
    std::thread autorun_strategyCancelOrder(&Strategy::strategyCancelOrder, this);
    autorun_updateOrderInfo.detach();
    autorun_updateTradeInfo.detach();
    autorun_strategyOrder.detach();
    autorun_strategyOffset.detach();
    autorun_strategyOffsetALL.detach();
    autorun_strategyCancelOrder.detach();
}

void axapi::Strategy::stopStrategy()
{
    char *t_strLogFuncName = "Strategy::stopStrategy";
    char t_strLog[500];
    sprintf_s(t_strLog, sizeof(t_strLog), "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    ResetEvent(m_hOpenFinished);
    m_blOpenRunning = false;
    WaitForSingleObject(m_hOpenFinished, INFINITY);

    ResetEvent(m_hOffsetFinished);
    ResetEvent(m_hCancelFinished);
    m_blOffsetRunning = false;
    m_blCancelRunning = false;
    WaitForSingleObject(m_hOffsetFinished, INFINITY);
    WaitForSingleObject(m_hCancelFinished, INFINITY);

    ResetEvent(m_hOffsetAllFinished);
    m_blOffsetALLRunning = true;
    WaitForSingleObject(m_hOffsetAllFinished, INFINITY);

    ResetEvent(m_hUpdateOrderFinished);
    ResetEvent(m_hUpdateTradeFinished);
    m_blShutdownFlag = true;
    WaitForSingleObject(m_hUpdateOrderFinished, INFINITY);
    WaitForSingleObject(m_hUpdateTradeFinished, INFINITY);

    saveData();
    m_blStrategyRunning = false;
}

void axapi::Strategy::updateOrderInfo()
{
    char *t_strLogFuncName = "Strategy::updateOrderInfo";
    char t_strLog[500];
    sprintf_s(t_strLog, sizeof(t_strLog), "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    APINamespace CThostFtdcOrderField t_objAPIOrderInfo;
    memset(&t_objAPIOrderInfo, '\0', sizeof(t_objAPIOrderInfo));

    try
    {
        while (!m_blShutdownFlag)
        {
            sprintf_s(t_strLog, sizeof(t_strLog), "%s:Running", t_strLogFuncName);
            LOG4CPLUS_TRACE(m_objLogger, t_strLog);

            m_nUpdateOrderTimes += 1;
            int t_ordersize = m_pTrade->sizeOrderList();
            sprintf_s(t_strLog, sizeof(t_strLog), "%s:sizeOrderList=%d", t_strLogFuncName, t_ordersize);
            LOG4CPLUS_TRACE(m_objLogger, t_strLog);

            for (int i = 1; i <= t_ordersize && !m_blShutdownFlag; i++)
            {
                t_objAPIOrderInfo = m_pTrade->getOrderInfo(i);
                if (t_objAPIOrderInfo.OrderSysID[0] == '\0')
                {
                    /// 超出范围
                    continue;
                }

                LOG4CPLUS_TRACE(m_objLogger, "-----------11111111--------------");
                /// 更新m_hashAllOrder,m_hashConfirmedOrder,m_vConfirmedOrder
                if (m_hashConfirmedOrder.find(t_objAPIOrderInfo.OrderSysID) == m_hashConfirmedOrder.end())
                    /// 当前委托未被确认,即m_hashAllOrder还未更新过信息,则新建m_hashConfirmedOrder,m_vConfirmedOrder
                {
                    /// 不在m_hashAllOrder中,则表示t_objAPIOrderInfo不属于该策略所为
                    if (m_hashAllOrder.find(t_objAPIOrderInfo.OrderRef) != m_hashAllOrder.end())
                    {
                        LOG4CPLUS_TRACE(m_objLogger, "-----------33333333--------------");
                        /// 更新m_hashAllOrder
                        strcpy_s(m_hashAllOrder[t_objAPIOrderInfo.OrderRef].OrderID, sizeof(UniversalChinaFutureTdOrderIDType), t_objAPIOrderInfo.OrderSysID);

                        ConfirmedOrder t_objConfirmedOrder;
                        //memset(&t_objConfirmedOrder, '\0', sizeof(t_objConfirmedOrder));
                        strcpy_s(t_objConfirmedOrder.OrderID, sizeof(t_objConfirmedOrder.OrderID), m_hashAllOrder[t_objAPIOrderInfo.OrderRef].OrderID);
                        strcpy_s(t_objConfirmedOrder.HoldTradeID, sizeof(t_objConfirmedOrder.HoldTradeID), m_hashAllOrder[t_objAPIOrderInfo.OrderRef].HoldTradeID);
                        strcpy_s(t_objConfirmedOrder.OrderRef, sizeof(t_objConfirmedOrder.OrderRef), m_hashAllOrder[t_objAPIOrderInfo.OrderRef].OrderRef);
                        t_objConfirmedOrder.OrderType = m_hashAllOrder[t_objAPIOrderInfo.OrderRef].OrderType;
                        t_objConfirmedOrder.OrderPrice = t_objAPIOrderInfo.LimitPrice;
                        t_objConfirmedOrder.OrderVolumnOriginal = t_objAPIOrderInfo.VolumeTotalOriginal;
                        t_objConfirmedOrder.OrderVolumeTraded = t_objAPIOrderInfo.VolumeTraded;
                        t_objConfirmedOrder.OrderVolumeTotal = t_objAPIOrderInfo.VolumeTotal;
                        t_objConfirmedOrder.OrderStatus = t_objAPIOrderInfo.OrderStatus;
                        strcpy_s(t_objConfirmedOrder.InsertTime, sizeof(t_objConfirmedOrder.InsertTime), t_objAPIOrderInfo.InsertTime);
                        t_objConfirmedOrder.TradeIDList.clear();
                        /// 创建m_hashConfirmedOrder
                        m_hashConfirmedOrder[t_objConfirmedOrder.OrderID] = m_vConfirmedOrder.size();
                        /// 创建m_vConfirmedOrder
                        m_vConfirmedOrder.push_back(t_objConfirmedOrder);
                        sprintf_s(t_strLog, sizeof(t_strLog), "%s:增加ConfirmedOrder[%d]_hashmap(%s)", t_strLogFuncName, m_vConfirmedOrder.size() - 1, t_objConfirmedOrder.OrderID);
                        LOG4CPLUS_DEBUG(m_objLogger, t_strLog);

                        /// 当前委托存在于m_hashUnpairedOffsetOrder中,需要进行匹配
                        if (m_hashUnpairedOffsetOrder.find(m_hashAllOrder[t_objAPIOrderInfo.OrderRef].HoldTradeID) != m_hashUnpairedOffsetOrder.end())
                        {
                            LOG4CPLUS_TRACE(m_objLogger, "-----------44444444--------------");
                            /// 当前委托对应开仓已被确认,可以进行匹配操作
                            if (m_hashConfirmedHoldTrade.find(m_hashAllOrder[t_objAPIOrderInfo.OrderRef].HoldTradeID) != m_hashConfirmedHoldTrade.end())
                            {
                                LOG4CPLUS_TRACE(m_objLogger, "-----------55555555--------------");
                                int t_HoldTradeIndex = m_hashConfirmedHoldTrade[m_hashAllOrder[t_objAPIOrderInfo.OrderRef].HoldTradeID];
                                /// 止盈平仓单对应到开仓
                                if (m_hashUnpairedOffsetOrder[m_hashAllOrder[t_objAPIOrderInfo.OrderRef].HoldTradeID].OffsetOrderType == OrderOffsetType_SP)
                                {
                                    strcpy_s(m_vConfirmedHoldTrade[t_HoldTradeIndex].SPOrderID, sizeof(m_vConfirmedHoldTrade[t_HoldTradeIndex].SPOrderID), m_hashAllOrder[t_objAPIOrderInfo.OrderRef].OrderID);
                                    sprintf_s(t_strLog, sizeof(t_strLog), "%s:ConfirmedHoldTrade[%d]_hashmap(%s)止盈平仓单被设置为(%s)", t_strLogFuncName, t_HoldTradeIndex, m_hashAllOrder[t_objAPIOrderInfo.OrderRef].HoldTradeID, m_hashAllOrder[t_objAPIOrderInfo.OrderRef].OrderID);
                                    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
                                }
                                /// 止损平仓单对应到开仓
                                else if (m_hashUnpairedOffsetOrder[m_hashAllOrder[t_objAPIOrderInfo.OrderRef].HoldTradeID].OffsetOrderType == OrderOffsetType_SL)
                                {
                                    m_vConfirmedHoldTrade[t_HoldTradeIndex].SLOrderID.push_back(m_hashAllOrder[t_objAPIOrderInfo.OrderRef].OrderID);
                                    sprintf_s(t_strLog, sizeof(t_strLog), "%s:ConfirmedHoldTrade[%d]_hashmap(%s)止损平仓单(%s)增加", t_strLogFuncName, t_HoldTradeIndex, m_hashAllOrder[t_objAPIOrderInfo.OrderRef].HoldTradeID, m_hashAllOrder[t_objAPIOrderInfo.OrderRef].OrderID);
                                    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
                                }
                                /// 清理m_hashUnpairedOffsetOrder中的信息
                                m_hashUnpairedOffsetOrder.erase(m_hashAllOrder[t_objAPIOrderInfo.OrderRef].HoldTradeID);

                                sprintf_s(t_strLog, sizeof(t_strLog), "%s:UnpairedOffsetOrder(%s)被剔除,剩余%d个未被确认", t_strLogFuncName, m_hashAllOrder[t_objAPIOrderInfo.OrderRef].HoldTradeID, m_hashUnpairedOffsetOrder.size());
                                LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
                                LOG4CPLUS_TRACE(m_objLogger, "-----------5a5a5a5a--------------");
                            }
                            LOG4CPLUS_TRACE(m_objLogger, "-----------4a4a4a4a--------------");
                        }
                        LOG4CPLUS_TRACE(m_objLogger, "-----------3a3a3a3a--------------");
                    }
                    LOG4CPLUS_TRACE(m_objLogger, "-----------1a1a1a1a--------------");
                }
                /// 当前委托已经确认存在,即m_hashAllOrder,m_hashConfirmedOrder已经更新过信息,则更新m_vConfirmedOrder
                else
                {
                    LOG4CPLUS_TRACE(m_objLogger, "-----------22222222--------------");
                    int t_ConfirmedOrderIndex = m_hashConfirmedOrder[t_objAPIOrderInfo.OrderSysID];
                    m_vConfirmedOrder[t_ConfirmedOrderIndex].OrderPrice = t_objAPIOrderInfo.LimitPrice;
                    m_vConfirmedOrder[t_ConfirmedOrderIndex].OrderVolumnOriginal = t_objAPIOrderInfo.VolumeTotalOriginal;
                    m_vConfirmedOrder[t_ConfirmedOrderIndex].OrderVolumeTraded = t_objAPIOrderInfo.VolumeTraded;
                    m_vConfirmedOrder[t_ConfirmedOrderIndex].OrderVolumeTotal = t_objAPIOrderInfo.VolumeTotal;
                    m_vConfirmedOrder[t_ConfirmedOrderIndex].OrderStatus = t_objAPIOrderInfo.OrderStatus;
                    LOG4CPLUS_TRACE(m_objLogger, "-----------2a2a2a2a--------------");
                }
            }
        }
    }
    catch (std::exception e)
    {
        sprintf_s(t_strLog, sizeof(t_strLog), "updateOrderInfo:%s", e.what());
        LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
    }

    sprintf_s(t_strLog, sizeof(t_strLog), "%s:stopped", t_strLogFuncName);
    LOG4CPLUS_INFO(m_objLogger, t_strLog);

    SetEvent(m_hUpdateOrderFinished);
}

void axapi::Strategy::updateTradeInfo()
{
    char *t_strLogFuncName = "Strategy::updateTradeInfo";
    char t_strLog[500];
    sprintf_s(t_strLog, sizeof(t_strLog), "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    TradeField t_objAPITradeInfo;
    memset(&t_objAPITradeInfo, '\0', sizeof(t_objAPITradeInfo));

    try
    {
        while (!m_blShutdownFlag)
        {
            sprintf_s(t_strLog, sizeof(t_strLog), "%s:Running", t_strLogFuncName);
            LOG4CPLUS_TRACE(m_objLogger, t_strLog);

            m_nUpdateTradeTimes += 1;
            int t_tradesize = m_pTrade->sizeTradeList();
            sprintf_s(t_strLog, sizeof(t_strLog), "%s:sizeTradeList=%d", t_strLogFuncName, t_tradesize);
            LOG4CPLUS_TRACE(m_objLogger, t_strLog);

            for (int i = 1; i <= t_tradesize && !m_blShutdownFlag; i++)
            {
                LOG4CPLUS_TRACE(m_objLogger, "---------------------------------11111111--------------");
                t_objAPITradeInfo = m_pTrade->getTradeInfo(i);
                if (t_objAPITradeInfo.apiTradeField.TradeID[0] == '\0')
                {
                    //超出范围
                    continue;
                }

                /// 更新m_vConfirmedOrder,m_hashConfirmedHoldTrade,m_vConfirmedHoldTrade
                if (m_hashConfirmedHoldTrade.find(t_objAPITradeInfo.apiTradeField.TradeID) == m_hashConfirmedHoldTrade.end())
                    /*
                    * 当前成交未被确认,m_hashConfirmedHoldTrade中不存在
                    * 如果为平仓则只更新m_vConfirmedOrder
                    * 如果为开仓则更新m_vConfirmedOrder,新建m_hashConfirmedHoldTrade,m_vConfirmedHoldTrade
                    */
                {
                    LOG4CPLUS_TRACE(m_objLogger, "---------------------------------33333333--------------");
                    /// 不在m_hashAllOrder中,则表示t_objAPITradeInfo不属于该策略所为
                    if (m_hashAllOrder.find(t_objAPITradeInfo.apiTradeField.OrderRef) != m_hashAllOrder.end())
                    {
                        LOG4CPLUS_TRACE(m_objLogger, "---------------------------------44444444--------------");
                        /// m_hashConfirmedOrder还未确认完成,就收到了成交信息,则跳过等待m_hashConfirmedOrder确认完成
                        if (m_hashConfirmedOrder.find(m_hashAllOrder[t_objAPITradeInfo.apiTradeField.OrderRef].OrderID) == m_hashConfirmedOrder.end())
                        {
                            continue;
                        }
                        else
                        {
                            LOG4CPLUS_TRACE(m_objLogger, "---------------------------------55555555--------------");
                            /// 更新m_vConfirmedOrder,如果已更新则跳过
                            int t_index = m_hashConfirmedOrder[m_hashAllOrder[t_objAPITradeInfo.apiTradeField.OrderRef].OrderID];
                            for (int j = 0; j < m_vConfirmedOrder[t_index].TradeIDList.size(); j++)
                            {
                                LOG4CPLUS_TRACE(m_objLogger, "---------------------------------66666666--------------");
                                /// m_vConfirmedOrder中已更新过TradeID,则跳过
                                if (strcmp(m_vConfirmedOrder[t_index].TradeIDList[j].c_str(), t_objAPITradeInfo.apiTradeField.TradeID) == 0)
                                {
                                    break;
                                }
                                /// m_vConfirmedOrder中未更新过TradeID,则更新
                                else
                                {
                                    if (j == m_vConfirmedOrder[t_index].TradeIDList.size() - 1)
                                    {
                                        m_vConfirmedOrder[t_index].TradeIDList.push_back(t_objAPITradeInfo.apiTradeField.TradeID);
                                    }
                                }
                                LOG4CPLUS_TRACE(m_objLogger, "---------------------------------6a6a6a6a--------------");
                            }


                            /// 如果为开仓则新建m_hashConfirmedHoldTrade,m_vConfirmedHoldTrade
                            if (t_objAPITradeInfo.apiTradeField.OffsetFlag == THOST_FTDC_OF_Open)
                            {
                                LOG4CPLUS_TRACE(m_objLogger, "---------------------------------77777777--------------");
                                ConfirmedHoldTrade t_objConfirmedHoldTrade;
                                //memset(&t_objConfirmedHoldTrade, '\0', sizeof(t_objConfirmedHoldTrade));
                                strcpy_s(t_objConfirmedHoldTrade.TradeID, sizeof(t_objConfirmedHoldTrade.TradeID), t_objAPITradeInfo.apiTradeField.TradeID);
                                strcpy_s(t_objConfirmedHoldTrade.InstrumentID, sizeof(t_objConfirmedHoldTrade.InstrumentID), t_objAPITradeInfo.apiTradeField.InstrumentID);
                                t_objConfirmedHoldTrade.Direction = t_objAPITradeInfo.apiTradeField.Direction;
                                t_objConfirmedHoldTrade.Price = t_objAPITradeInfo.apiTradeField.Price;
                                t_objConfirmedHoldTrade.Volumn = t_objAPITradeInfo.apiTradeField.Volume;
                                strcpy_s(t_objConfirmedHoldTrade.TradeTime, sizeof(t_objConfirmedHoldTrade.TradeTime), t_objAPITradeInfo.apiTradeField.TradeTime);
                                t_objConfirmedHoldTrade.AvailableVolumn = t_objAPITradeInfo.apiTradeField.Volume;
                                t_objConfirmedHoldTrade.OffsetVolumn = 0;
                                t_objConfirmedHoldTrade.HighestProfitPrice = t_objAPITradeInfo.apiTradeField.Price;
                                t_objConfirmedHoldTrade.TradeStatus = TradeStatus_Hold;
                                memset(t_objConfirmedHoldTrade.SPOrderID, '\0', sizeof(t_objConfirmedHoldTrade.SPOrderID));
                                t_objConfirmedHoldTrade.SLOrderID.clear();

                                /// 创建m_hashConfirmedHoldTrade
                                m_hashConfirmedHoldTrade[t_objAPITradeInfo.apiTradeField.TradeID] = m_vConfirmedHoldTrade.size();
                                /// 创建m_vConfirmedHoldTrade
                                m_vConfirmedHoldTrade.push_back(t_objConfirmedHoldTrade);

                                sprintf_s(t_strLog, sizeof(t_strLog), "%s:增加ConfirmedHoldTrade[%d]_hash(%s)", t_strLogFuncName, m_vConfirmedHoldTrade.size() - 1, t_objAPITradeInfo.apiTradeField.TradeID);
                                LOG4CPLUS_INFO(m_objLogger, t_strLog);
                                LOG4CPLUS_TRACE(m_objLogger, "---------------------------------7a7a7a7a--------------");
                            }
                            LOG4CPLUS_TRACE(m_objLogger, "---------------------------------5a5a5a5a--------------");
                        }
                        LOG4CPLUS_TRACE(m_objLogger, "---------------------------------4a4a4a4a--------------");
                    }
                    LOG4CPLUS_TRACE(m_objLogger, "---------------------------------3a3a3a3a--------------");
                }
                /*
                * 当前成交已被确认则通过平仓信息更新持仓数据
                */
                else
                {
                    LOG4CPLUS_TRACE(m_objLogger, "---------------------------------22222222--------------");
                    updateHoldTrade(m_hashConfirmedHoldTrade[t_objAPITradeInfo.apiTradeField.TradeID]);
                    LOG4CPLUS_TRACE(m_objLogger, "---------------------------------2a2a2a2a--------------");
                }
                LOG4CPLUS_TRACE(m_objLogger, "---------------------------------1a1a1a1a--------------");
            }
        }
    }
    catch (std::exception e)
    {
        sprintf_s(t_strLog, sizeof(t_strLog), "updateTradeInfo:%s", e.what());
        LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
    }

    sprintf_s(t_strLog, sizeof(t_strLog), "%s:stopped", t_strLogFuncName);
    LOG4CPLUS_INFO(m_objLogger, t_strLog);

    SetEvent(m_hUpdateTradeFinished);
}

void axapi::Strategy::updateHoldTrade(UniversalChinaFutureTdSequenceType in_HoldTradeIndex)
{
    char *t_strLogFuncName = "Strategy::updateHoldTrade";
    char t_strLog[500];
    sprintf_s(t_strLog, sizeof(t_strLog), "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    UniversalChinaFutureTdVolumnType t_HoldTradeOffsetVolumn = 0;

    /// 持仓已全平则跳过
    if (m_vConfirmedHoldTrade[in_HoldTradeIndex].TradeStatus == TradeStatus_OffsetALL)
    {
        return;
    }

    /// SP平仓信息存在则检索更新
    if (m_vConfirmedHoldTrade[in_HoldTradeIndex].SPOrderID[0] != '\0')
    {
        int t_SPOrderIndex = m_hashConfirmedOrder[m_vConfirmedHoldTrade[in_HoldTradeIndex].SPOrderID];
        /// 无论委托类型是什么只记录已经成交的部分
        t_HoldTradeOffsetVolumn += m_vConfirmedOrder[t_SPOrderIndex].OrderVolumeTraded;
    }

    /// SL平仓信息存在则检索更新
    for (int i = 0; i < m_vConfirmedHoldTrade[in_HoldTradeIndex].SLOrderID.size(); i++)
    {
        int t_SLOrderIndex = m_hashConfirmedOrder[m_vConfirmedHoldTrade[in_HoldTradeIndex].SLOrderID[i]];
        /// 无论委托类型是什么只记录已经成交的部分
        t_HoldTradeOffsetVolumn += m_vConfirmedOrder[t_SLOrderIndex].OrderVolumeTraded;
    }

    /*
    * 更新OffsetVolumn,AvailableVolumn
    * 更新最大盈利价位HighestProfitPrice
    * 如果Volumn==OffsetVolumn,则仓全部平完,更新持仓状态
    */
    // 如果可以查询到当前合约行情则更新最大盈利价位
    axapi::MarketDataField t_objCurrentPrice;
    axapi::MarketDataField* t_currentPrice = m_pMarketQuotation->getCurrentPrice(m_vConfirmedHoldTrade[in_HoldTradeIndex].InstrumentID);
    if (t_currentPrice != NULL)
    {
        memcpy_s(&t_objCurrentPrice, sizeof(t_objCurrentPrice), t_currentPrice, sizeof(t_objCurrentPrice));
        if ((m_vConfirmedHoldTrade[in_HoldTradeIndex].Direction == THOST_FTDC_D_Buy
            && m_vConfirmedHoldTrade[in_HoldTradeIndex].HighestProfitPrice < t_objCurrentPrice.LastPrice)
            || (m_vConfirmedHoldTrade[in_HoldTradeIndex].Direction == THOST_FTDC_D_Sell
                && m_vConfirmedHoldTrade[in_HoldTradeIndex].HighestProfitPrice > t_objCurrentPrice.LastPrice))
        {
            m_vConfirmedHoldTrade[in_HoldTradeIndex].HighestProfitPrice = t_objCurrentPrice.LastPrice;
        }
    }
    m_vConfirmedHoldTrade[in_HoldTradeIndex].OffsetVolumn = t_HoldTradeOffsetVolumn;
    m_vConfirmedHoldTrade[in_HoldTradeIndex].AvailableVolumn = m_vConfirmedHoldTrade[in_HoldTradeIndex].Volumn - m_vConfirmedHoldTrade[in_HoldTradeIndex].OffsetVolumn < 0 ? 0 : m_vConfirmedHoldTrade[in_HoldTradeIndex].Volumn - m_vConfirmedHoldTrade[in_HoldTradeIndex].OffsetVolumn;
    m_vConfirmedHoldTrade[in_HoldTradeIndex].TradeStatus = m_vConfirmedHoldTrade[in_HoldTradeIndex].Volumn == m_vConfirmedHoldTrade[in_HoldTradeIndex].OffsetVolumn ? TradeStatus_OffsetALL : TradeStatus_Hold;
}

void axapi::Strategy::strategyOrder()
{
    char *t_strLogFuncName = "Strategy::strategyOrder";
    char t_strLog[500];
    sprintf_s(t_strLog, sizeof(t_strLog), "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    bool t_blOpenFlag = false;
    std::string t_strOffsetMessage;
    char t_strContract[20];
    memset(t_strContract, '\0', sizeof(t_strContract));
    int t_nDirection, t_nOffsetFlag, t_nOrderTypeFlag, t_nOrderAmount;
    double t_dOrderPrice;
    long t_plOpenOrderRef;
    UniversalChinaFutureTdOrderRefType t_objOpenOrderRef;

    try
    {
        while (m_blOpenRunning)
        {
            sprintf_s(t_strLog, sizeof(t_strLog), "%s:Running", t_strLogFuncName);
            LOG4CPLUS_TRACE(m_objLogger, t_strLog);

            myStrategy(&t_blOpenFlag, &t_strOffsetMessage, t_strContract, &t_nDirection, &t_nOffsetFlag, &t_nOrderTypeFlag, &t_nOrderAmount, &t_dOrderPrice);

            if (t_blOpenFlag)
            {
                m_pTrade->MyOrdering(t_strContract, t_nDirection, t_nOffsetFlag, t_nOrderTypeFlag, t_nOrderAmount, t_dOrderPrice, &t_plOpenOrderRef);
                sprintf_s(t_objOpenOrderRef, sizeof(t_objOpenOrderRef), "%ld", t_plOpenOrderRef);

                /// 开仓信息加入m_hashAllOrder,等待后续确认
                AllOrder t_objAllOrder;
                memset(&t_objAllOrder, '\0', sizeof(t_objAllOrder));
                strcpy_s(t_objAllOrder.OrderRef, sizeof(t_objAllOrder.OrderRef), t_objOpenOrderRef);
                t_objAllOrder.OrderType = OrderStatus_Open;
                t_objAllOrder.updateOrderRoundSequence = m_nUpdateOrderTimes;

                m_hashAllOrder[t_objOpenOrderRef] = t_objAllOrder;

                sprintf_s(t_strLog, sizeof(t_strLog), "%s:下单%s_%s", t_strLogFuncName, t_objOpenOrderRef, t_strOffsetMessage.c_str());
                LOG4CPLUS_INFO(m_objLogger, t_strLog);
            }
            Sleep(m_nOrderingCheckWaitMillseconds);
        }
    }
    catch (std::exception e)
    {
        sprintf_s(t_strLog, sizeof(t_strLog), "strategyOrder:%s", e.what());
        LOG4CPLUS_ERROR(m_objLogger, t_strLog);
    }

    sprintf_s(t_strLog, sizeof(t_strLog), "%s:stopped", t_strLogFuncName);
    LOG4CPLUS_INFO(m_objLogger, t_strLog);

    SetEvent(m_hOpenFinished);
}

void axapi::Strategy::strategyOffset()
{
    char *t_strLogFuncName = "Strategy::strategyOffset";
    char t_strLog[500];
    sprintf_s(t_strLog, sizeof(t_strLog), "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    bool t_blOffsetFlag;
    std::string t_strOffsetMessage;

    try
    {
        while (m_blOffsetRunning)
        {
            sprintf_s(t_strLog, sizeof(t_strLog), "%s:Running", t_strLogFuncName);
            LOG4CPLUS_TRACE(m_objLogger, t_strLog);

            /// 遍历所有的持仓,找到还未平仓的持仓
            for (int i = 0; m_blOffsetRunning && i < m_vConfirmedHoldTrade.size(); i++)
            {
                LOG4CPLUS_TRACE(m_objLogger, "------------------------------------------------------------------11111111--------------");

                t_blOffsetFlag = false;
                t_strOffsetMessage.clear();
                if (m_vConfirmedHoldTrade[i].TradeStatus == TradeStatus_Hold)
                {
                    LOG4CPLUS_TRACE(m_objLogger, "------------------------------------------------------------------22222222--------------");
                    myOffsetStrategy(m_vConfirmedHoldTrade[i], &t_blOffsetFlag, &t_strOffsetMessage);
                    /// 需要立即平仓
                    if (t_blOffsetFlag)
                    {
                        LOG4CPLUS_TRACE(m_objLogger, "------------------------------------------------------------------33333333--------------");
                        /// 如果有止盈则撤止盈单 下止损单
                        if (m_vConfirmedHoldTrade[i].SPOrderID[0] != '\0')
                        {
                            LOG4CPLUS_TRACE(m_objLogger, "------------------------------------------------------------------55555555--------------");
                            bool t_blCancelSPOrder = true;
                            while (t_blCancelSPOrder)
                            {
                                switch (m_vConfirmedOrder[m_hashConfirmedOrder[m_vConfirmedHoldTrade[i].SPOrderID]].OrderStatus)
                                {
                                case THOST_FTDC_OST_NoTradeQueueing:
                                case THOST_FTDC_OST_PartTradedQueueing:
                                    m_pTrade->MyCancelOrder(m_vConfirmedHoldTrade[i].SPOrderID);
                                    Sleep(100);
                                    break;
                                default:
                                    t_blCancelSPOrder = false;
                                    break;
                                }
                            }

                            m_vConfirmedHoldTrade[i].SLOrderID.push_back(m_vConfirmedHoldTrade[i].SPOrderID);
                            memset(&m_vConfirmedHoldTrade[i].SPOrderID, '\0', sizeof(m_vConfirmedHoldTrade[i].SPOrderID));
                            LOG4CPLUS_TRACE(m_objLogger, "------------------------------------------------------------------5a5a5a5a--------------");
                        }
                        /// 如果下过止盈单但报单未确认则删除配对信息
                        else if (m_hashUnpairedOffsetOrder.find(m_vConfirmedHoldTrade[i].TradeID) != m_hashUnpairedOffsetOrder.end()
                            && m_hashUnpairedOffsetOrder[m_vConfirmedHoldTrade[i].TradeID].OffsetOrderType == OrderOffsetType_SP)
                        {
                            LOG4CPLUS_TRACE(m_objLogger, "------------------------------------------------------------------66666666--------------");
                            /// UnpairedOffsetOrder中的未匹配委托已经过多轮匹配扔未匹配上,可直接删除
                            if (m_hashAllOrder[m_hashUnpairedOffsetOrder[m_vConfirmedHoldTrade[i].TradeID].OrderRef].updateOrderRoundSequence + UnpairedOffsetOrderCheckSequenceDiff <= m_nUpdateOrderTimes)
                            {
                                m_hashUnpairedOffsetOrder.erase(m_vConfirmedHoldTrade[i].TradeID);
                            }
                            else
                            {
                                continue;
                            }
                            LOG4CPLUS_TRACE(m_objLogger, "------------------------------------------------------------------6a6a6a6a--------------");
                        }

                        /// 确认没有止盈后,则直接下止损单
                        UniversalChinaFutureTdOrderRefType t_objSLOrderRef;
                        long t_lSLOrderRef;
                        m_pTrade->MyOrdering(m_vConfirmedHoldTrade[i].InstrumentID,
                            m_vConfirmedHoldTrade[i].Direction == THOST_FTDC_D_Buy ? ORDER_DIRECTION_SELL : ORDER_DIRECTION_BUY,
                            ORDER_OFFSETFLAG_OFFSET_TODAY,
                            ORDER_LIMITPRICE,
                            m_vConfirmedHoldTrade[i].AvailableVolumn,
                            m_vConfirmedHoldTrade[i].Direction == THOST_FTDC_D_Buy ? m_pMarketQuotation->getCurrentPrice(m_vConfirmedHoldTrade[i].InstrumentID)->BidPrice1 : m_pMarketQuotation->getCurrentPrice(m_vConfirmedHoldTrade[i].InstrumentID)->AskPrice1,
                            &t_lSLOrderRef);
                        sprintf_s(t_objSLOrderRef, sizeof(t_objSLOrderRef), "%ld", t_lSLOrderRef);

                        /// 止损平仓信息加入m_hashUnpairedOffsetOrder,等待后续匹配
                        UnpairedOffsetOrder t_objUnpairedOffsetOrder;
                        memset(&t_objUnpairedOffsetOrder, '\0', sizeof(t_objUnpairedOffsetOrder));
                        strcpy_s(t_objUnpairedOffsetOrder.TradeID, sizeof(t_objUnpairedOffsetOrder.TradeID), m_vConfirmedHoldTrade[i].TradeID);
                        strcpy_s(t_objUnpairedOffsetOrder.OrderRef, sizeof(t_objUnpairedOffsetOrder.OrderRef), t_objSLOrderRef);
                        t_objUnpairedOffsetOrder.OffsetOrderType = OrderOffsetType_SL;

                        m_hashUnpairedOffsetOrder[m_vConfirmedHoldTrade[i].TradeID] = t_objUnpairedOffsetOrder;

                        /// 止盈平仓信息加入m_hashAllOrder,等待后续确认
                        AllOrder t_objAllOrder;
                        memset(&t_objAllOrder, '\0', sizeof(t_objAllOrder));
                        strcpy_s(t_objAllOrder.HoldTradeID, sizeof(t_objAllOrder.HoldTradeID), m_vConfirmedHoldTrade[i].TradeID);
                        strcpy_s(t_objAllOrder.OrderRef, sizeof(t_objAllOrder.OrderRef), t_objSLOrderRef);
                        t_objAllOrder.OrderType = OrderStatus_SLOffset;
                        t_objAllOrder.updateOrderRoundSequence = m_nUpdateOrderTimes;

                        m_hashAllOrder[t_objSLOrderRef] = t_objAllOrder;
                        sprintf_s(t_strLog, sizeof(t_strLog), "%s:下单m_vConfirmedHoldTrade[%d](%s)止损平仓单m_hashAllOrder(%s)", t_strLogFuncName, i, m_vConfirmedHoldTrade[i].TradeID, t_objSLOrderRef);
                        LOG4CPLUS_INFO(m_objLogger, t_strLog);
                        LOG4CPLUS_TRACE(m_objLogger, "------------------------------------------------------------------3a3a3a3a--------------");
                    }
                    /// 如果不需要平仓
                    else
                    {
                        LOG4CPLUS_TRACE(m_objLogger, "------------------------------------------------------------------44444444--------------");
                        /// 没有止盈,设止盈
                        if (m_vConfirmedHoldTrade[i].SPOrderID[0] == '\0'
                            && m_vConfirmedHoldTrade[i].SLOrderID.size() == 0
                            && m_hashUnpairedOffsetOrder.find(m_vConfirmedHoldTrade[i].TradeID) == m_hashUnpairedOffsetOrder.end())
                        {
                            LOG4CPLUS_TRACE(m_objLogger, "------------------------------------------------------------------77777777--------------");
                            bool t_blSPOffsetFlag = false;
                            double t_dbSPOrderPrice;
                            getPreOffsetPrice(m_vConfirmedHoldTrade[i], &t_blSPOffsetFlag, &t_dbSPOrderPrice);
                            if (t_blSPOffsetFlag)
                            {
                                UniversalChinaFutureTdOrderRefType t_objSPOrderRef;
                                long t_lSPOrderRef;

                                /// 下单获得OrderRef
                                m_pTrade->MyOrdering(m_vConfirmedHoldTrade[i].InstrumentID,
                                    (m_vConfirmedHoldTrade[i].Direction == THOST_FTDC_D_Buy ? ORDER_DIRECTION_SELL : ORDER_DIRECTION_BUY),
                                    ORDER_OFFSETFLAG_OFFSET_TODAY,
                                    ORDER_LIMITPRICE,
                                    m_vConfirmedHoldTrade[i].AvailableVolumn,
                                    t_dbSPOrderPrice,
                                    &t_lSPOrderRef);
                                sprintf_s(t_objSPOrderRef, sizeof(t_objSPOrderRef), "%ld", t_lSPOrderRef);

                                /// 止盈平仓信息加入m_hashUnpairedOffsetOrder,等待后续匹配
                                UnpairedOffsetOrder t_objUnpairedOffsetOrder;
                                memset(&t_objUnpairedOffsetOrder, '\0', sizeof(t_objUnpairedOffsetOrder));
                                strcpy_s(t_objUnpairedOffsetOrder.TradeID, sizeof(t_objUnpairedOffsetOrder.TradeID), m_vConfirmedHoldTrade[i].TradeID);
                                strcpy_s(t_objUnpairedOffsetOrder.OrderRef, sizeof(t_objUnpairedOffsetOrder.OrderRef), t_objSPOrderRef);
                                t_objUnpairedOffsetOrder.OffsetOrderType = OrderOffsetType_SP;

                                m_hashUnpairedOffsetOrder[m_vConfirmedHoldTrade[i].TradeID] = t_objUnpairedOffsetOrder;

                                /// 止盈平仓信息加入m_hashAllOrder,等待后续确认
                                AllOrder t_objAllOrder;
                                memset(&t_objAllOrder, '\0', sizeof(t_objAllOrder));
                                strcpy_s(t_objAllOrder.HoldTradeID, sizeof(t_objAllOrder.HoldTradeID), m_vConfirmedHoldTrade[i].TradeID);
                                strcpy_s(t_objAllOrder.OrderRef, sizeof(t_objAllOrder.OrderRef), t_objSPOrderRef);
                                t_objAllOrder.OrderType = OrderStatus_SPOffset;
                                t_objAllOrder.updateOrderRoundSequence = m_nUpdateOrderTimes;

                                m_hashAllOrder[t_objSPOrderRef] = t_objAllOrder;

                                sprintf_s(t_strLog, sizeof(t_strLog), "%s:下单m_vConfirmedHoldTrade[%d](%s)止盈平仓单m_hashAllOrder(%s)", t_strLogFuncName, i, m_vConfirmedHoldTrade[i].TradeID, t_objSPOrderRef);
                                LOG4CPLUS_INFO(m_objLogger, t_strLog);
                            }
                            LOG4CPLUS_TRACE(m_objLogger, "------------------------------------------------------------------7a7a7a7a--------------");
                        }
                        LOG4CPLUS_TRACE(m_objLogger, "------------------------------------------------------------------4a4a4a4a--------------");
                    }
                    LOG4CPLUS_TRACE(m_objLogger, "------------------------------------------------------------------2a2a2a2a--------------");
                }
                LOG4CPLUS_TRACE(m_objLogger, "------------------------------------------------------------------1a1a1a1a--------------");
            }
        }
    }
    catch (std::exception e)
    {
        sprintf_s(t_strLog, sizeof(t_strLog), "strategyOffset:%s", e.what());
        LOG4CPLUS_ERROR(m_objLogger, t_strLog);
    }

    sprintf_s(t_strLog, sizeof(t_strLog), "%s:stopped", t_strLogFuncName);
    LOG4CPLUS_INFO(m_objLogger, t_strLog);

    SetEvent(m_hOffsetFinished);
}

void axapi::Strategy::strategyCancelOrder()
{
    char *t_strLogFuncName = "Strategy::strategyCancelOrder";
    char t_strLog[500];
    sprintf_s(t_strLog, sizeof(t_strLog), "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    //用于获取时间
    time_t nowtime;
    tm *curtime;
    int curMinutes, t_OrderInsertTime;
    char t_OrderInfoHour[3];
    char t_OrderInfoMinutes[3];
    char t_OrderInfoSeconds[3];
    //是否撤单标志
    bool t_CancelAction;

    try
    {
        while (m_blCancelRunning)
        {
            sprintf_s(t_strLog, sizeof(t_strLog), "%s:Running", t_strLogFuncName);
            LOG4CPLUS_TRACE(m_objLogger, t_strLog);

            for (int i = 0; m_blCancelRunning && i < m_vConfirmedOrder.size(); i++)
            {
                switch (m_vConfirmedOrder[i].OrderStatus)
                {
                case THOST_FTDC_OST_PartTradedQueueing:
                case THOST_FTDC_OST_NoTradeQueueing:
                {
                    /// 预埋止盈单跳过
                    if (m_vConfirmedOrder[i].OrderType == OrderStatus_SPOffset)
                    {
                        continue;
                    }
                    /// 止损单重新下单
                    else if (m_vConfirmedOrder[i].OrderType == OrderStatus_SLOffset || m_vConfirmedOrder[i].OrderType == OrderStatus_Open)
                    {
                        /*
                        * 是否撤单
                        * 1.n秒撤单
                        */
#pragma region
                        nowtime = time(NULL);
                        curtime = localtime(&nowtime);
                        t_OrderInfoHour[0] = m_vConfirmedOrder[i].InsertTime[0];
                        t_OrderInfoHour[1] = m_vConfirmedOrder[i].InsertTime[1];
                        t_OrderInfoHour[2] = '\0';
                        t_OrderInfoMinutes[0] = m_vConfirmedOrder[i].InsertTime[3];
                        t_OrderInfoMinutes[1] = m_vConfirmedOrder[i].InsertTime[4];
                        t_OrderInfoMinutes[2] = '\0';
                        t_OrderInfoSeconds[0] = m_vConfirmedOrder[i].InsertTime[6];
                        t_OrderInfoSeconds[1] = m_vConfirmedOrder[i].InsertTime[7];
                        t_OrderInfoSeconds[2] = '\0';
                        if (curtime->tm_hour < 20)
                        {
                            curMinutes = (curtime->tm_hour + 24) * 3600 + curtime->tm_min * 60 + curtime->tm_sec * 1;
                        }
                        else
                        {
                            curMinutes = (curtime->tm_hour + 0) * 3600 + curtime->tm_min * 60 + curtime->tm_sec * 1;
                        }
                        if (atoi(t_OrderInfoHour) < 20)
                        {
                            t_OrderInsertTime = (atoi(t_OrderInfoHour) + 24) * 3600 + atoi(t_OrderInfoMinutes) * 60 + atoi(t_OrderInfoSeconds);
                        }
                        else
                        {
                            t_OrderInsertTime = (atoi(t_OrderInfoHour) + 0) * 3600 + atoi(t_OrderInfoMinutes) * 60 + atoi(t_OrderInfoSeconds);
                        }
                        sprintf_s(t_strLog, sizeof(t_strLog), "%s:curMinutes:%d,|%d:%d:%d", t_strLogFuncName, curMinutes, curtime->tm_hour, curtime->tm_min, curtime->tm_sec);
                        LOG4CPLUS_TRACE(m_objLogger, t_strLog);
                        sprintf_s(t_strLog, sizeof(t_strLog), "%s:m_vConfirmedOrder[%d]:%d,%s|%s|%s|%s:%s:%s", t_strLogFuncName, i, t_OrderInsertTime, m_vConfirmedOrder[i].OrderID, m_vConfirmedOrder[i].OrderRef, m_vConfirmedOrder[i].InsertTime, t_OrderInfoHour, t_OrderInfoMinutes, t_OrderInfoSeconds);
                        LOG4CPLUS_TRACE(m_objLogger, t_strLog);
                        sprintf_s(t_strLog, sizeof(t_strLog), "%s:m_vConfirmedOrder[%d](%s):curMinutes:%d,t_OrderInsertTime:%d,m_nCancelWaitSeconds:%d", t_strLogFuncName, i, m_vConfirmedOrder[i].OrderID, curMinutes, t_OrderInsertTime, m_nCancelWaitSeconds);
                        LOG4CPLUS_DEBUG(m_objLogger, t_strLog);

                        if (curMinutes - t_OrderInsertTime >= m_nCancelWaitSeconds)
                        {
                            t_CancelAction = true;
                            sprintf_s(t_strLog, sizeof(t_strLog), "%s:撤单m_vConfirmedOrder[%d](%s)", t_strLogFuncName, i, m_vConfirmedOrder[i].OrderID);
                            LOG4CPLUS_INFO(m_objLogger, t_strLog);
                        }
                        else
                        {
                            t_CancelAction = false;
                        }
#pragma endregion
                        if (t_CancelAction)
                        {
                            /// 撤单
                            bool t_blCanelNOSPOrder = true;
                            while (t_blCanelNOSPOrder)
                            {
                                switch (m_vConfirmedOrder[i].OrderStatus)
                                {
                                case THOST_FTDC_OST_PartTradedQueueing:
                                case THOST_FTDC_OST_NoTradeQueueing:
                                    m_pTrade->MyCancelOrder(m_vConfirmedOrder[i].OrderID);
                                    Sleep(100);
                                    break;
                                default:
                                    t_blCanelNOSPOrder = false;
                                    break;
                                }
                            }

                            /// ERROR 更新不及时，导致撤不可撤单。后续报单平错误持仓

                            /// 如果是止损平仓报单则继续报入,开仓不做处理
                            if (m_vConfirmedOrder[i].OrderType == OrderStatus_SLOffset)
                            {
                                UniversalChinaFutureTdOrderRefType t_objSLOrderRef;
                                long t_lSLOrderRef;
                                m_pTrade->MyOrdering(m_vConfirmedHoldTrade[m_hashConfirmedHoldTrade[m_vConfirmedOrder[i].HoldTradeID]].InstrumentID,
                                    m_vConfirmedHoldTrade[m_hashConfirmedHoldTrade[m_vConfirmedOrder[i].HoldTradeID]].Direction == THOST_FTDC_D_Buy ? ORDER_DIRECTION_SELL : ORDER_DIRECTION_BUY,
                                    ORDER_OFFSETFLAG_OFFSET_TODAY,
                                    ORDER_AGAINSTPRICE,
                                    m_vConfirmedOrder[i].OrderVolumeTotal,
                                    m_pMarketQuotation->getCurrentPrice(m_vConfirmedHoldTrade[m_hashConfirmedHoldTrade[m_vConfirmedOrder[i].HoldTradeID]].InstrumentID)->AskPrice1,
                                    &t_lSLOrderRef);
                                sprintf_s(t_objSLOrderRef, sizeof(t_objSLOrderRef), "%ld", t_lSLOrderRef);

                                /// 止损平仓信息加入m_hashUnpairedOffsetOrder,等待后续匹配
                                UnpairedOffsetOrder t_objUnpairedOffsetOrder;
                                memset(&t_objUnpairedOffsetOrder, '\0', sizeof(t_objUnpairedOffsetOrder));
                                strcpy_s(t_objUnpairedOffsetOrder.TradeID, sizeof(t_objUnpairedOffsetOrder.TradeID), m_vConfirmedOrder[i].HoldTradeID);
                                strcpy_s(t_objUnpairedOffsetOrder.OrderRef, sizeof(t_objUnpairedOffsetOrder.OrderRef), t_objSLOrderRef);
                                t_objUnpairedOffsetOrder.OffsetOrderType = OrderOffsetType_SL;

                                m_hashUnpairedOffsetOrder[m_vConfirmedHoldTrade[i].TradeID] = t_objUnpairedOffsetOrder;

                                /// 止损平仓信息加入m_hashAllOrder,等待后续确认
                                AllOrder t_objAllOrder;
                                memset(&t_objAllOrder, '\0', sizeof(t_objAllOrder));
                                strcpy_s(t_objAllOrder.HoldTradeID, sizeof(t_objAllOrder.HoldTradeID), m_vConfirmedOrder[i].HoldTradeID);
                                strcpy_s(t_objAllOrder.OrderRef, sizeof(t_objAllOrder.OrderRef), t_objSLOrderRef);
                                t_objAllOrder.OrderType = OrderStatus_SLOffset;
                                t_objAllOrder.updateOrderRoundSequence = m_nUpdateOrderTimes;

                                m_hashAllOrder[t_objSLOrderRef] = t_objAllOrder;
                                sprintf_s(t_strLog, sizeof(t_strLog), "%s:止损平仓撤单后补报(%s)", t_strLogFuncName, t_objSLOrderRef);
                                LOG4CPLUS_INFO(m_objLogger, t_strLog);
                            }
                        }
                    }
                    break;
                }
                default:
                    break;
                }
            }
        }
    }
    catch (std::exception e)
    {
        sprintf_s(t_strLog, sizeof(t_strLog), "%s:%s", t_strLogFuncName, e.what());
        LOG4CPLUS_ERROR(m_objLogger, t_strLog);
    }

    sprintf_s(t_strLog, sizeof(t_strLog), "%s:stopped", t_strLogFuncName);
    LOG4CPLUS_INFO(m_objLogger, t_strLog);

    SetEvent(m_hCancelFinished);
}

void axapi::Strategy::strategyOffsetALL()
{
    char *t_strLogFuncName = "Strategy::strategyOffsetALL";
    char t_strLog[500];
    sprintf_s(t_strLog, sizeof(t_strLog), "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    while (true)
    {
        sprintf_s(t_strLog, sizeof(t_strLog), "%s:Running", t_strLogFuncName);
        LOG4CPLUS_TRACE(m_objLogger, t_strLog);

        if (m_blOffsetALLRunning)
        {
            /// 先撤挂单
            int t_nUnCanceledOrder = 1;
            while (t_nUnCanceledOrder != 0)
            {
                t_nUnCanceledOrder = 0;
                for (int i = 0; i < m_vConfirmedOrder.size(); i++)
                {
                    switch (m_vConfirmedOrder[i].OrderStatus)
                    {
                    case THOST_FTDC_OST_NoTradeQueueing:
                    case THOST_FTDC_OST_PartTradedQueueing:
                        m_pTrade->MyCancelOrder(m_vConfirmedOrder[i].OrderID);
                        t_nUnCanceledOrder++;
                        break;
                    default:
                        break;
                    }
                }
            }
            sprintf_s(t_strLog, sizeof(t_strLog), "%s:清仓检索持仓记录%d条", t_strLogFuncName, m_vConfirmedHoldTrade.size());
            LOG4CPLUS_DEBUG(m_objLogger, t_strLog);

            for (int i = 0; i < m_vConfirmedHoldTrade.size(); i++)
            {
                if (m_vConfirmedHoldTrade[i].TradeStatus == TradeStatus_Hold)
                {
                    axapi::MarketDataField* t_currentPrice = m_pMarketQuotation->getCurrentPrice(m_vConfirmedHoldTrade[i].InstrumentID);
                    // 没行情暂时不处理
                    if (t_currentPrice == NULL)
                    {
                        continue;
                    }

                    switch (m_vConfirmedHoldTrade[i].Direction)
                    {
                    case THOST_FTDC_D_Buy:
                    {
                        //std::cout << "清仓:卖平仓单" << m_vConfirmedHoldTrade[i].InstrumentID << "|" << m_vConfirmedHoldTrade[i].AvailableVolumn << "|" << t_currentPrice->LastPrice << "|" << t_currentPrice->AskPrice1 << "|" << t_currentPrice->BidPrice1 << std::endl;
                        sprintf_s(t_strLog, sizeof(t_strLog), "%s:清仓卖平仓TradeID(%s),InstrumentID(%s),AvailableVolumn(%d),LastPrice(%lf),AskPrice1(%lf),BidPrice1(%lf)", t_strLogFuncName, m_vConfirmedHoldTrade[i].TradeID, m_vConfirmedHoldTrade[i].InstrumentID, m_vConfirmedHoldTrade[i].AvailableVolumn, t_currentPrice->LastPrice, t_currentPrice->AskPrice1, t_currentPrice->BidPrice1);
                        LOG4CPLUS_INFO(m_objLogger, t_strLog);
                        if (m_pTrade->MyOrdering(m_vConfirmedHoldTrade[i].InstrumentID, ORDER_DIRECTION_SELL, ORDER_OFFSETFLAG_OFFSET_TODAY, ORDER_AGAINSTPRICE, m_vConfirmedHoldTrade[i].AvailableVolumn, t_currentPrice->BidPrice1) >= 0)
                        {
                        }

                    }
                    break;
                    case THOST_FTDC_D_Sell:
                    {
                        //std::cout << "清仓:买平仓单" << m_vConfirmedHoldTrade[i].InstrumentID << "|" << m_vConfirmedHoldTrade[i].AvailableVolumn << "|" << t_currentPrice->LastPrice << "|" << t_currentPrice->AskPrice1 << "|" << t_currentPrice->BidPrice1 << std::endl;
                        sprintf_s(t_strLog, sizeof(t_strLog), "%s:清仓买平仓TradeID(%s),InstrumentID(%s),AvailableVolumn(%d),LastPrice(%lf),AskPrice1(%lf),BidPrice1(%lf)", t_strLogFuncName, m_vConfirmedHoldTrade[i].TradeID, m_vConfirmedHoldTrade[i].InstrumentID, m_vConfirmedHoldTrade[i].AvailableVolumn, t_currentPrice->LastPrice, t_currentPrice->AskPrice1, t_currentPrice->BidPrice1);
                        LOG4CPLUS_INFO(m_objLogger, t_strLog);
                        if (m_pTrade->MyOrdering(m_vConfirmedHoldTrade[i].InstrumentID, ORDER_DIRECTION_BUY, ORDER_OFFSETFLAG_OFFSET_TODAY, ORDER_AGAINSTPRICE, m_vConfirmedHoldTrade[i].AvailableVolumn, t_currentPrice->AskPrice1) >= 0)
                        {
                        }
                    }
                    break;
                    }
                }
            }
            sprintf_s(t_strLog, sizeof(t_strLog), "%s:stopped", t_strLogFuncName);
            LOG4CPLUS_INFO(m_objLogger, t_strLog);

            SetEvent(m_hOffsetAllFinished);
            return;
        }
        Sleep(100);
    }
}

bool axapi::Strategy::strategyHoldCompare()
{
    char *t_strLogFuncName = "Strategy::strategyHoldCompare";
    char t_strLog[500];
    sprintf_s(t_strLog, sizeof(t_strLog), "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    std::hash_map<std::string, UniversalChinaFutureTdVolumnType> t_hashAPIPosition;
    std::hash_map<std::string, UniversalChinaFutureTdVolumnType> t_hashPosition;
    APINamespace CThostFtdcInvestorPositionDetailField t_objPositionDetailInfo;

    int t_nRequestID = m_pTrade->queryCustSTKHoldDetail();
    while (!m_pTrade->checkCompletedQueryRequestID(t_nRequestID))
    {
        std::cout << "waiting for position detail infomation" << std::endl;
        Sleep(100);
    }
    int t_positiondetailsize = m_pTrade->sizePositionDetailList();
    std::cout << "OffsetALL:sizePositionDetailList=" << t_positiondetailsize << std::endl;
    for (int i = 1; i <= t_positiondetailsize; i++)
    {
        t_objPositionDetailInfo = m_pTrade->getPositionDetailInfo(i);
        if (t_hashAPIPosition.find(t_objPositionDetailInfo.InstrumentID) == t_hashAPIPosition.end())
        {
            t_hashAPIPosition[t_objPositionDetailInfo.InstrumentID] = t_objPositionDetailInfo.Volume;
        }
        else
        {
            t_hashAPIPosition[t_objPositionDetailInfo.InstrumentID] += t_objPositionDetailInfo.Volume;
        }
    }

    for (int i = 0; i < m_vConfirmedHoldTrade.size(); i++)
    {
        if (m_vConfirmedHoldTrade[i].TradeStatus == TradeStatus_Hold)
        {
            if (t_hashPosition.find(m_vConfirmedHoldTrade[i].InstrumentID) == t_hashPosition.end())
            {
                t_hashPosition[m_vConfirmedHoldTrade[i].InstrumentID] = m_vConfirmedHoldTrade[i].AvailableVolumn;
            }
            else
            {
                t_hashPosition[m_vConfirmedHoldTrade[i].InstrumentID] += m_vConfirmedHoldTrade[i].AvailableVolumn;
            }
        }
    }

    /// 输出不一致
    for (std::hash_map<std::string, UniversalChinaFutureTdVolumnType>::iterator i = t_hashPosition.begin(); i != t_hashPosition.end(); i++)
    {
        if (t_hashAPIPosition.find(i->first) == t_hashAPIPosition.end())
        {
            std::cout << i->first.c_str() << ":" << i->second << "<->" << 0 << std::endl;
        }
        else
        {
            std::cout << i->first.c_str() << ":" << i->second << "<->" << t_hashAPIPosition[i->first] << std::endl;
        }
    }
    return true;
}

bool axapi::Strategy::getOpenRunning()
{
    return m_blOpenRunning;
}

bool axapi::Strategy::getOffsetRunning()
{
    return m_blOffsetRunning;
}

#ifdef STRATEGY_EXE
void axapi::Strategy::myStrategy(bool *ot_blOpenFlag, std::string *ot_strOpenMsg, char *ot_strContract, int *ot_nDirection, int *ot_nOffsetFlag, int *ot_nOrderTypeFlag, int *ot_nOrderAmount, double *ot_dOrderPrice)
{
    char *t_strLogFuncName = "Strategy::myStrategy";
    char t_strLog[500];
    sprintf_s(t_strLog, sizeof(t_strLog), "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    *ot_blOpenFlag = false;

    char *t_instrument = "i1906";

    m_pMarketQuotation->subMarketDataSingle(t_instrument);
    if (m_nOpenCount < 3)
    {
        Sleep(1000);
        *ot_strOpenMsg = "test open";
        sprintf_s(ot_strContract, 10, t_instrument);
        *ot_nDirection = ORDER_DIRECTION_SELL;
        *ot_nOffsetFlag = ORDER_OFFSETFLAG_OPEN;
        *ot_nOrderTypeFlag = ORDER_LIMITPRICE;
        *ot_nOrderAmount = 1;
        if (m_pMarketQuotation->getCurrentPrice(t_instrument) == NULL)
        {
            return;
        }
        MarketDataField *p_price = m_pMarketQuotation->getCurrentPrice(t_instrument);
        *ot_dOrderPrice = p_price->BidPrice1;
        if (*ot_dOrderPrice <= 0)
        {
            return;
        }
        *ot_blOpenFlag = true;
        m_nOpenCount++;
    }
}
#endif STRATEGY_EXE

#ifdef STRATEGY_EXE
void axapi::Strategy::myOffsetStrategy(ConfirmedHoldTrade in_objHoldTrade, bool *ot_blOffsetFlag, std::string *ot_strOffsetMsg)
{
    char *t_strLogFuncName = "Strategy::myOffsetStrategy";
    char t_strLog[500];
    sprintf_s(t_strLog, sizeof(t_strLog), "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    *ot_blOffsetFlag = false;
    if (in_objHoldTrade.SPOrderID[0] != '\0')
    {
        *ot_strOffsetMsg = "SPOrader已设定";
        *ot_blOffsetFlag = true;
    }
}
#endif STRATEGY_EXE

#ifdef STRATEGY_EXE
void axapi::Strategy::getPreOffsetPrice(ConfirmedHoldTrade in_objHoldTrade, bool *ot_blSPOffsetFlag, double *ot_dbSPOffsetPrice)
{
    char *t_strLogFuncName = "Strategy::getPreOffsetPrice";
    char t_strLog[500];
    sprintf_s(t_strLog, sizeof(t_strLog), "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    *ot_blSPOffsetFlag = false;

    *ot_dbSPOffsetPrice = in_objHoldTrade.Price - 10;
    *ot_blSPOffsetFlag = true;
}
#endif STRATEGY_EXE