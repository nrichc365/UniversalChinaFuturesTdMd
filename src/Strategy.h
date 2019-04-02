#ifndef _STRATEGY_H_
#define _STRATEGY_H_
#pragma once

#ifdef STRATEGY_EXE
#define STRATEGY_EXPORT
#else
#ifdef STRATEGY_EXP
#define STRATEGY_EXPORT __declspec(dllexport)
#else
#define STRATEGY_EXPORT __declspec(dllimport)
#endif STRATEGY_EXP
#endif STRATEGY_EXE

#define _SCL_SECURE_NO_WARNINGS

#include <MarketQuotationAPI.h>
#include <TradeAPI.h>
#include <string>
#include <vector>
#include <hash_map>

namespace axapi
{
    typedef APINamespace TThostFtdcOrderRefType     UniversalChinaFutureTdOrderRefType;
    typedef APINamespace TThostFtdcOrderSysIDType   UniversalChinaFutureTdOrderIDType;
    typedef APINamespace TThostFtdcOrderStatusType  UniversalChinaFutureTdOrderStatusType;
    typedef char                                    UniversalChinaFutureTdOrderTypeType;
    typedef char                                    UniversalChinaFutureTdOffsetOrderTypeType;
    typedef APINamespace TThostFtdcTradeIDType      UniversalChinaFutureTdTradeIDType;
    typedef char                                    UniversalChinaFutureTdTradeStatusType;
    typedef unsigned int                            UniversalChinaFutureTdSequenceType;
    typedef unsigned int                            UniversalChinaFutureTdVolumnType;
    typedef double                                  UniversalChinaFutureTdPriceType;
    typedef APINamespace TThostFtdcInstrumentIDType UniversalChinaFutureTdInstrumentIDType;
    typedef APINamespace TThostFtdcDirectionType    UniversalChinaFutureTdDirectionType;
    typedef APINamespace TThostFtdcTimeType         UniversalChinaFutureTdTimeType;

    //---------------------------------------------------------------
    //OrderStatus Definition
    // 开仓
#define OrderStatus_Open 'a'
    // 止盈平仓
#define OrderStatus_SPOffset 'b'
    // 止损平仓
#define OrderStatus_SLOffset 'c'
    //---------------------------------------------------------------
    //TradeStatus Definition
    // 持有
#define TradeStatus_Hold 'a'
    // 已平
#define TradeStatus_OffsetALL 'b'
    //---------------------------------------------------------------
    //UnpairedOffsetOrder Definition
    // 止盈
#define OrderOffsetType_SP 'a'
    // 止损
#define OrderOffsetType_SL 'b'
    //---------------------------------------------------------------

    /// 未与回报确认过的委托信息,从策略中发出的保单委托都有记录
    struct AllOrder
    {
        UniversalChinaFutureTdOrderRefType  OrderRef;
        UniversalChinaFutureTdSequenceType  updateOrderRoundSequence;
        UniversalChinaFutureTdOrderIDType   OrderID;
        UniversalChinaFutureTdOrderTypeType OrderType;
        /// 如果委托为平仓,则标识对应的成交ID
        UniversalChinaFutureTdTradeIDType   HoldTradeID;
    };

    /// 已与回报确认过的委托信息,可以作为策略中委托信息查询
    struct ConfirmedOrder
    {
        UniversalChinaFutureTdOrderRefType    OrderRef;
        UniversalChinaFutureTdOrderIDType     OrderID;
        UniversalChinaFutureTdOrderTypeType   OrderType;
        UniversalChinaFutureTdOrderStatusType OrderStatus;
        UniversalChinaFutureTdPriceType       OrderPrice;
        UniversalChinaFutureTdVolumnType      OrderVolumnOriginal;
        UniversalChinaFutureTdVolumnType      OrderVolumeTraded;
        UniversalChinaFutureTdVolumnType      OrderVolumeTotal;
        UniversalChinaFutureTdTimeType        InsertTime;
        /// 如果委托为平仓,则标识对应的成交ID
        UniversalChinaFutureTdTradeIDType     HoldTradeID;
        std::vector<UniversalChinaFutureTdTradeIDType> TradeIDList;
    };

    /// 已与回报确认过的开仓成交,可以作为策略中持仓信息查询
    struct ConfirmedHoldTrade
    {
        UniversalChinaFutureTdTradeIDType              TradeID;
        UniversalChinaFutureTdOrderIDType              SPOrderID;
        std::vector<UniversalChinaFutureTdOrderIDType> SLOrderID;
        UniversalChinaFutureTdTradeStatusType          TradeStatus;
        UniversalChinaFutureTdInstrumentIDType         InstrumentID;
        UniversalChinaFutureTdDirectionType            Direction;
        UniversalChinaFutureTdVolumnType               Volumn;
        UniversalChinaFutureTdPriceType                Price;
        UniversalChinaFutureTdVolumnType               AvailableVolumn;
        UniversalChinaFutureTdVolumnType               OffsetVolumn;
    };

    /// 平仓后记录,未与持仓信息匹配的平仓信息
    struct UnpairedOffsetOrder
    {
        UniversalChinaFutureTdTradeIDType         TradeID;
        UniversalChinaFutureTdOrderRefType        OrderRef;
        UniversalChinaFutureTdOffsetOrderTypeType OffsetOrderType;
    };

    ///

    class STRATEGY_EXPORT Strategy
    {
    public:
        /// 初始化行情与交易接口
        int initializeAPI(axapi::MarketQuotationAPI*, axapi::TradeAPI*);
        /// 策略执行状态切换
        void start() { startStrategy(); };
        void stop() { stopStrategy(); };
        void pause() { pauseStrategy(); };
        void continu() { continueStrategy(); };
        /// 初始化
        Strategy(void);
        ~Strategy(void);

    protected:
        /// 外接行情数据
        axapi::MarketQuotationAPI *m_pMarketQuotation;
        axapi::TradeAPI *m_pTrade;
        /// 设置接口
        int setAPI(axapi::MarketQuotationAPI*, axapi::TradeAPI*);
        /// 获得运行状态
        bool getOpenRunning();
        bool getOffsetRunning();

        /// TODO:策略主体
        void myStrategy(bool *ot_blOpenFlag, std::string *ot_strOpenMsg, char *ot_strContract, int *ot_nDirection, int *ot_nOffsetFlag, int *ot_nOrderTypeFlag, int *ot_nOrderAmount, double *ot_dOrderPrice);
        /// TODO:平仓主体
        void myOffsetStrategy(struct ConfirmedHoldTrade in_objHoldTrade, bool *ot_blOffsetFlag, std::string *ot_strOffsetMsg);
        /// TODO:预埋单价位获得
        void getPreOffsetPrice(struct ConfirmedHoldTrade in_objHoldTrade, bool *ot_blSPOffsetFlag, double *ot_dbSPOffsetPrice);

    private:
        /// 记录已报所有委托,报入失败的也在其中
        std::hash_map<std::string/*UniversalChinaFutureTdOrderRefType*/, struct AllOrder> m_hashAllOrder;
        /// 记录被确认已报入的委托,m_hashConfirmedOrder属于m_hashAllOrder的子集,m_hashConfirmedOrder为m_vConfirmedOrder的索引
        std::hash_map<std::string/*UniversalChinaFutureTdOrderIDType*/, UniversalChinaFutureTdSequenceType>  m_hashConfirmedOrder;
        /// 记录被确认已报入的委托
        std::vector<struct ConfirmedOrder> m_vConfirmedOrder;
        /// 记录开仓的成交,作为持仓信息来使用,m_hashConfirmedHoldTrade为m_vConfirmedHoldTrade的索引
        std::hash_map<std::string/*UniversalChinaFutureTdTradeIDType*/, UniversalChinaFutureTdSequenceType> m_hashConfirmedHoldTrade;
        /// 记录开仓的成交,作为持仓信息来使用
        std::vector<struct ConfirmedHoldTrade> m_vConfirmedHoldTrade;
        /// 记录未与持仓信息匹配的平仓信息,临时中转使用,待确认后即删除
        std::hash_map<std::string/*UniversalChinaFutureTdTradeIDType*/, struct UnpairedOffsetOrder> m_hashUnpairedOffsetOrder;

        /// 执行状态
        bool m_blStrategyRunning;
        bool m_blOffsetALLRunning;
        bool m_blOffsetRunning;
        bool m_blOpenRunning;
        bool m_blCancelRunning;
        bool m_blShutdownFlag;
        HANDLE m_hOpenFinished;
        HANDLE m_hOffsetFinished;
        HANDLE m_hCancelFinished;
        HANDLE m_hOffsetAllFinished;
        HANDLE m_hUpdateOrderFinished;
        HANDLE m_hUpdateTradeFinished;

        /// 策略参数
        unsigned int m_nCancelWaitSeconds;

        /// 委托更新次数
        unsigned int m_nUpdateOrderTimes;
        /// 成交更新次数
        unsigned int m_nUpdateTradeTimes;

        /// 策略执行开始
        void startStrategy();
        /// 策略执行停止
        void stopStrategy();
        /// 策略执行暂停
        void pauseStrategy() {};
        /// 策略执行重启
        void continueStrategy() {};

        /// 检索委托回报
        void updateOrderInfo();
        /// 检索成交回报
        void updateTradeInfo();
        /// 根据平仓委托情况更新持仓情况
        void updateHoldTrade(UniversalChinaFutureTdSequenceType);
        /// 下单进程,调用自定义策略
        void strategyOrder();
        /// 平仓进程,调用自定义平仓策略,以及执行默认的策略
        void strategyOffset();
        /// 清仓进程
        void strategyOffsetALL();
        /// 撤单进程
        void strategyCancelOrder();
        /// 持仓比较
        bool strategyHoldCompare();
        /// 运行数据保存
        void saveData();

    };
}

#endif _STRATEGY_H_