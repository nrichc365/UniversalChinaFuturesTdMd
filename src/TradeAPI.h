/*
* version 1.0.0
* created by niuchao
* on Apr. 13th 2018
* 该版本合成BaseAPI与BaseAPI_CTP,旨在通过设置开关来切换不同柜台的版本,
* 或者实现同一个下单程序可以同时对接不同的柜台,例如：下单在金仕达,行情来自CTP
*  开关为 KSV6T_TRADEAPI, CTP_TRADEAPI
*/
#ifndef _TRADEAPI_H_
#define _TRADEAPI_H_
#pragma once

#ifdef TradeAPI_EXE
#define TradeAPI_EXPORT
#else
#ifdef TradeAPI_EXP
#define TradeAPI_EXPORT __declspec(dllexport)
#else
#define TradeAPI_EXPORT __declspec(dllimport)
#endif TradeAPI_EXP
#endif TradeAPI_EXE

#include <log4cplus/logger.h>
#include <log4cplus/configurator.h>
#include <log4cplus/helpers/loglog.h>
#include <log4cplus/helpers/stringhelper.h>
#include <log4cplus/helpers/sleep.h>
#include <log4cplus/loggingmacros.h>
#include <Windows.h>
#include <vector>

#ifdef SQLITE3DATA
#include    "CppSQLite3/CppSQLite3.h"
#endif  SQLITE3DATA

#ifndef KSV6T_TRADEAPI
#ifndef CTP_TRADEAPI
#define     APINamespace
#else   CTP_TRADEAPI
#include    <CTPTdMd_API_common/ThostFtdcTraderApi.h>
#define     APINamespace
#endif  CTP_TRADEAPI
#else   KSV6T_TRADEAPI
#include    <KSv6tTdMd_API_common/KSTradeAPI.h>
#define     APINamespace KingstarAPI::
#endif  KSV6T_TRADEAPI

namespace axapi
{
    /*
    * 自定义数据结构
    */
#pragma region
    /// 成交记录结构 
    struct TradeField
    {
        /// API接口规范
        APINamespace CThostFtdcTradeField apiTradeField;
        /// 剩余手数
        int Volumn;
        /// 最高盈利价
        double Price;
    };
#pragma endregion

    class TradeAPI_EXPORT TradeAPI : public APINamespace CThostFtdcTraderSpi
    {
        /*
        * 初始化
        */
#pragma region
    private:
        /// 用于日志,初始化必须设置
        log4cplus::Logger m_root;
        /// 用于日志,初始化必须设置
        log4cplus::Logger m_objLogger;
        /// 账号信息 经纪商ID
        APINamespace TThostFtdcBrokerIDType m_strBrokerID;
        /// 账号信息 客户号
        APINamespace TThostFtdcUserIDType m_strUserID;
        /// 账号信息 客户密码
        APINamespace TThostFtdcPasswordType m_strPassword;
        /// 当前交易日
        APINamespace TThostFtdcDateType m_strTradingDay;
        /// 启动开始设置日志相关配置
        int initialLog();
        /// 设置用户信息
        int setUserInfo(APINamespace TThostFtdcBrokerIDType in_strBrokerID, APINamespace TThostFtdcUserIDType in_strUserID, APINamespace TThostFtdcPasswordType in_strPassword);
        /// 链接服务器
        int initiate(char* in_strFrontAddr);
    public:
        /// 用此函数初始化,则状态不正常,必须使用另外一种重载过的初始化函数
        TradeAPI();
        /// 初始化日志,数据库,建立连接
        TradeAPI(APINamespace TThostFtdcBrokerIDType in_strBrokerID, APINamespace TThostFtdcUserIDType in_strUserID, APINamespace TThostFtdcPasswordType in_strPassword, char* in_strFrontAddr);
        /// TradeAPI(char* in_strBrokerID, char* in_strUserID, char* in_strPassword, char* in_strFrontAddr);
        /// 断开服务器,断开数据库
        ~TradeAPI();
#pragma endregion

        /*
        * 当前API接口状态
        * 接口运行变量
        */
#pragma region
    private:
        /// API接口运行状态
        char m_chStatus;
        /// a pointer of CThostFtdcMduserApi instance
        APINamespace CThostFtdcTraderApi* m_pUserApi;
        /// 事件,用于锁
        HANDLE m_hInitEvent;
        /// 最大报单引用orderef,报单时需保证该值不重复
        /// 启动时更新该变量,收到委托回报时更新该变量p
        APINamespace TThostFtdcOrderRefType m_nOrderRef;
        /// 请求ID,请求ID不重复
        int m_nRequestID;
        /// 存放已完成客户端查询请求对应的RequestID
        std::vector<int> m_nCompleted_Query_RequestID;
        /// 状态操作,用于初始化时
        int setStatus(char);
        /// 将完成查询请求ID加入完成ID列表
        void setCompletedQueryRequestID(int);
    public:
        /// 返回状态,用于判断下单部分状态
        char getStatus();
        /// 查看所提供查询ID是否完成
        bool checkCompletedQueryRequestID(int);
#pragma endregion

        /*
        * 交易请求
        */
#pragma region
    private:
        /// 委托报单结构
        APINamespace CThostFtdcInputOrderField m_objOrder;
        /// 最新价,用于下单函数中取价格保证成交使用
        APINamespace CThostFtdcDepthMarketDataField m_LatestPrice;
        /// 设置下单结构
        void setOrderInfo(char* in_strContract, int in_nDirection, int in_nOffsetFlag, int in_nOrderTypeFlag, int in_nOrderAmount, double in_dOrderPrice);
        /// 获得合约当前价,用于下单函数中取价格
        double getMarketData(char* in_strContract);
    public:
        /// 查询指定合约的行情,结果进入内存结果,可通过MarketQuotationAPI替换该功能 
        int queryMarketData(char* in_strContract);
        /// 获取queryMarketData所查询得到的合约的最新行情
        APINamespace CThostFtdcDepthMarketDataField getLatestPrice();
        /// 下单 返回Requestid,参数依次为in_strContract合约、in_nDirection方向、in_nOffsetFlag开平标志、in_nOrderTypeFlag报单类型、in_nOrderAmount报单数量、in_dOrderPrice报单价格、in_plOrderRef引用指针
        int MyOrdering(char* in_strContract, int in_nDirection, int in_nOffsetFlag, int in_nOrderTypeFlag, int in_nOrderAmount, double in_dOrderPrice, long *in_plOrderRef = NULL);
        /// 撤单 in_OrderSysID为委托号,需要通过查询m_vOrderList获得
        int MyCancelOrder(char* in_OrderSysID);
#pragma endregion

        /*
        * 查询请求
        *   外部查询接口,返回Requestid,是否完成查询需要和m_nCompleted_Query_RequestID
        */
#pragma region
    private:
    public:
        /// 查询成交,结果入库或者进入内存结果,ret:Requestid
        int queryCustDone();
        /// 查询资金,结果入库或者进入内存结果,ret:Requestid
        int queryCustFund();
        /// 查询委托,结果入库或者进入内存结果,ret:Requestid
        int queryCustOrder();
        /// 查询持仓,结果入库或者进入内存结果,ret:Requestid
        int queryCustSTKHoldDetail();
        /// 查询持仓明细,结果入库或者进入内存结果,ret:Requestid
        int queryCustSTKHold();
        /// 查询合约信息,结果入库或者进入内存结果,ret:Requestid
        int queryInstrument();
#pragma endregion

        /*
        * 数据存储
        *   处理数据到指定存储单元,sqlite3DB或者memory
        */
#pragma region
    private:
        /// 处理所有插入或者更新cust_order的操作,ret:0 正常
        int insertorUpdateOrder(APINamespace CThostFtdcOrderField *pOrder);
        /// 处理所有插入或者更新cust_done的操作,ret:0 正常
        int insertorUpdateTrade(APINamespace CThostFtdcTradeField *pTrade);
        /// 处理所有插入或者更新cust_fund的操作,ret:0 正常
        int insertorUpdateFund(APINamespace CThostFtdcTradingAccountField *pTradingAccount);
        /// 处理所有插入或者更新cust_stock_hold_detail的操作,ret:0 正常
        int insertorUpdateSTKHoldDetail(APINamespace CThostFtdcInvestorPositionDetailField *pInvestorPositionDetail);
        /// 处理所有插入或者更新cust_stock_hold_detail的操作,ret:0 正常
        int insertorUpdateSTKHold(APINamespace CThostFtdcInvestorPositionField *pInvestorPosition);
        /// 处理所有插入或者更新market_data的操作,ret:0 正常
        int insertorUpdateMarketData(APINamespace CThostFtdcDepthMarketDataField *pDepthMarketData);
        /// 处理所有插入或者更新instrument的操作,ret:0 正常
        int insertorUpdateInstrument(APINamespace CThostFtdcInstrumentField *pInstrument);
        /// 处理所有插入或者更新instrumentStatus的操作,ret:0 正常
        int insertorUpdateInstrumentStatus(APINamespace CThostFtdcInstrumentStatusField *pInstrumentStatus);
    public:
#pragma endregion

        /*
        * 数据库数据
        */
#pragma region
#ifdef SQLITE3DATA
    private:
        /// 交易信息存放于SQLite数据库中
        CppSQLite3DB m_objDB;
        /// （已废弃）用于向外传递查询结果
        CppSQLite3Query m_objQryResult;
        /// 数据文件不存在创建数据相关结构,ret:0 正常
        int createDB();
        /// 初始化时调用清理历史记录,ret:0 正常
        int clearDB();
        /// 初始化时调用清理历史记录,ret:0 正常
        int clearDB(char* in_strTablename);
        /// 启动开始连接数据库等操作
        int initialDB();
    public:
        /// 用于向外部传递查询结果 p_SQLiteQueryResult为查询结果 in_pSQLStatement为查询语句
        void queryDB(CppSQLite3Query* p_SQLiteQueryResult, char* in_pSQLStatement);
#endif SQLITE3DATA
#pragma endregion

        /*
        * 内存数据
        */
#pragma region
#ifdef MEMORYDATA
    private:
        /// 持仓信息
        std::vector<APINamespace CThostFtdcInvestorPositionField> m_vInvestorPositionList;
        /// 持仓明细信息
        std::vector<APINamespace CThostFtdcInvestorPositionDetailField> m_vInvestorPositionDetailList;
        /// 委托信息
        std::vector<APINamespace CThostFtdcOrderField> m_vOrderList;
        /// 成交信息,即当日仓位
        std::vector<TradeField> m_vTradeList;
        /// 合约信息结构
        std::vector<APINamespace CThostFtdcInstrumentField> m_vInstrumentList;
        /// 品种交易状态
        std::vector<APINamespace CThostFtdcInstrumentStatusField> m_vInstrumentStatusList;
    public:
        /// 获得委托数据集合的个数
        int sizeOrderList();
        /// 获得成交数据集合的个数
        int sizeTradeList();
        /// 获得合约信息数据集合的个数
        int sizeInstrumentList();
        /// 获得持仓明细信息数据集合的个数
        int sizePositionDetailList();
        /// 获得合约信息数据集合的个数
        int sizeInstrumentStatusList();
        /// 通过委托遍历位置获得委托信息,in_OrderListPostion委托遍历位置
        APINamespace CThostFtdcOrderField getOrderInfo(int in_nOrderListPosition);
        /// 通过成交遍历位置获得成交信息,in_TradeListPostion成交遍历位置
        TradeField getTradeInfo(int in_nTradeListPosition);
        /// 通过合约信息遍历位置获得合约信息,in_nInstrumentListPostion合约遍历位置
        APINamespace CThostFtdcInstrumentField getInstrumentInfo(int in_nInstrumentListPosition);
        /// 通过合约代码获得合约信息,in_strContract合约代码
        APINamespace CThostFtdcInstrumentField getInstrumentInfo(char* in_strContract);
        /// 通过持仓明细信息遍历位置获得持仓明细信息,in_nPositionDetailListPosition持仓明细遍历位置
        APINamespace CThostFtdcInvestorPositionDetailField getPositionDetailInfo(int in_nPositionDetailListPosition);
        /// 设置成交信息,主要用于重置成本价格与剩余数量,目前只支持Volumn与Priceset,ret:0 正常
        int setTradeInfo(int in_TradeListPostion, char* in_Type, double in_dbvalue=0, int in_nvalue=0);
        /// 通过合约交易状态遍历位置获得合约交易状态,in_nInstrumentStatusListPostion合约遍历位置
        APINamespace CThostFtdcInstrumentStatusField getInstrumentStatusInfo(int in_nInstrumentStatusListPostion);
        /// 通过合约代码获得合约交易状态
        APINamespace CThostFtdcInstrumentStatusField getInstrumentStatusInfo(char* in_strContract);
#endif MEMORYDATA
#pragma endregion

        /*
        * 测试使用
        */
#pragma region
#ifdef TEST
    private:
        void test();
        void test2();
        void test3();
    public:
#endif TEST
#pragma endregion

        /*
        * 柜台接口提供
        */
#pragma region
    private:
        /// 链接
        virtual void OnFrontConnected();
        /// When the connection between client and the CTP server	disconnected,the follwing function will be called.
        virtual void OnFrontDisconnected(int nReason);
        /// 公有流
        virtual void OnRtnInstrumentStatus(APINamespace CThostFtdcInstrumentStatusField *pInstrumentStatus);
        /// 报单录入错误回报
        virtual void OnErrRtnOrderInsert(APINamespace CThostFtdcInputOrderField *pInputOrder, APINamespace CThostFtdcRspInfoField *pRspInfo);
        /// 报单操作错误回报
        virtual void OnErrRtnOrderAction(APINamespace CThostFtdcOrderActionField *pOrderAction, APINamespace CThostFtdcRspInfoField *pRspInfo);
        /// After receiving the login request from the client,the CTP server will send the following response to notify the client whether the login success or not.
        virtual void OnRspUserLogin(APINamespace CThostFtdcRspUserLoginField *pRspUserLogin, APINamespace CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
        /// investor response
        virtual void OnRspQryInvestor(APINamespace CThostFtdcInvestorField *pInvestor, APINamespace CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
        /// tradeaccount response
        virtual void OnRspQryTradingAccount(APINamespace CThostFtdcTradingAccountField *pTradingAccount, APINamespace CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
        /// RspQryExchange
        virtual void OnRspQryExchange(APINamespace CThostFtdcExchangeField *pExchange, APINamespace CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
        /// RspQryInstrument
        virtual void OnRspQryInstrument(APINamespace CThostFtdcInstrumentField *pInstrument, APINamespace CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
        /// QryInvestorPositionDetail response
        virtual void OnRspQryInvestorPositionDetail(APINamespace CThostFtdcInvestorPositionDetailField *pInvestorPositionDetail, APINamespace CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
        /// QryInstrumentMarginRate response
        virtual void OnRspQryInstrumentMarginRate(APINamespace CThostFtdcInstrumentMarginRateField *pInstrumentMarginRate, APINamespace CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
        /// QryInstrumentCommissionRate response
        virtual void OnRspQryInstrumentCommissionRate(APINamespace CThostFtdcInstrumentCommissionRateField *pInstrumentCommissionRate, APINamespace CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
        /// output the DepthMarketData result 
        virtual void OnRspQryDepthMarketData(APINamespace CThostFtdcDepthMarketDataField *pDepthMarketData, APINamespace CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
        /// order insertion response 
        virtual void OnRspOrderInsert(APINamespace CThostFtdcInputOrderField *pInputOrder, APINamespace CThostFtdcRspInfoField *pRspInfo, int  nRequestID, bool bIsLast);
        /// order insertion return 
        virtual void OnRtnOrder(APINamespace CThostFtdcOrderField *pOrder);
        ///trade return
        virtual void OnRtnTrade(APINamespace CThostFtdcTradeField *pTrade);
        /// the error notification caused by client request
        virtual void OnRspError(APINamespace CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
        /// output the order action result 
        virtual void OnRspOrderAction(APINamespace CThostFtdcInputOrderActionField *pInputOrderAction, APINamespace CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
        /// qryorder return
        virtual void OnRspQryOrder(APINamespace CThostFtdcOrderField *pOrder, APINamespace CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
        /// qrytrade return
        virtual void OnRspQryTrade(APINamespace CThostFtdcTradeField *pTrade, APINamespace CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
        /// QryInvestorPosition return
        virtual void OnRspQryInvestorPosition(APINamespace CThostFtdcInvestorPositionField *pInvestorPosition, APINamespace CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
        /// logout return
        virtual void OnRspUserLogout(APINamespace CThostFtdcUserLogoutField *pUserLogout, APINamespace CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
        // 结算单查询请求响应
        virtual void OnRspQrySettlementInfo(APINamespace CThostFtdcSettlementInfoField *pSettlementInfo, APINamespace CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
        // 结算单确认时间查询
        virtual void OnRspQrySettlementInfoConfirm(APINamespace CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, APINamespace CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
        // 结算单确认
        virtual void OnRspSettlementInfoConfirm(APINamespace CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, APINamespace CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
#ifdef CTP_TRADEAPI
        ///交易所公告通知
        virtual void OnRtnBulletin(APINamespace CThostFtdcBulletinField *pBulletin);
        ///交易通知
        virtual void OnRtnTradingNotice(APINamespace CThostFtdcTradingNoticeInfoField *pTradingNoticeInfo);
#endif CTP_TRADEAPI
        ///执行宣告通知
        virtual void OnRtnExecOrder(APINamespace CThostFtdcExecOrderField *pExecOrder);
    public:
#pragma endregion
    };
}
#endif _TRADEAPI_H_