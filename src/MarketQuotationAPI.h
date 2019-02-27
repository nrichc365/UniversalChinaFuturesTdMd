#ifndef _MARKETQUOTATIONAPI_H_
#define _MARKETQUOTATIONAPI_H_
#pragma once

#define _SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#ifdef MarketQuotationAPI_EXE
#define MarketQuotationAPI_EXPORT
#else
#ifdef MarketQuotationAPI_EXP
#define MarketQuotationAPI_EXPORT __declspec(dllexport)
#else
#define MarketQuotationAPI_EXPORT __declspec(dllimport)
#endif MarketQuotationAPI_EXP
#endif MarketQuotationAPI_EXE

#include <log4cplus/logger.h>
#include <log4cplus/configurator.h>
#include <log4cplus/helpers/loglog.h>
#include <log4cplus/helpers/stringhelper.h>
#include <log4cplus/helpers/sleep.h>
#include <log4cplus/loggingmacros.h>
#include <Windows.h>
#include <time.h>
#include <vector>
#include <hash_map>
#ifdef KLINESTORAGE
#include <vector>
#endif KLINESTORAGE
#include <string>

#ifndef KSV6T_TRADEAPI
#ifndef CTP_TRADEAPI
#define     APINamespace
#else   CTP_TRADEAPI
#include    <CTPTdMd_API_common/ThostFtdcMdApi.h>
#define     APINamespace
#endif  CTP_TRADEAPI
#else   KSV6T_TRADEAPI
#include    <KSv6tTdMd_API_common/KSMarketDataAPI.h>
#define     APINamespace KingstarAPI::
#endif  KSV6T_TRADEAPI

#ifdef HQDATAFILE
#define DATAFILE
#endif HQDATAFILE
#ifdef KDATAFILE
#define DATAFILE
#endif KDATAFILE

#ifdef DATAFILE
#include <DataIntoFiles\DataIntoFiles.h>
#endif DATAFILE

#ifndef WAITFORCLOSEFILE
#define WAITFORCLOSEFILE 2000
#endif  WAITFORCLOSEFILE

namespace axapi
{
    typedef int APIStatus;
#define APISTATUS_DISCONNECTED  9999
#define APISTATUS_CONNECTED     1000

    struct Contract
    {
        APINamespace TThostFtdcInstrumentIDType InstrumentID;
    };

    struct MarketDataField
    {
        ///交易日
        APINamespace TThostFtdcDateType	TradingDay;
        ///合约代码
        APINamespace TThostFtdcInstrumentIDType	InstrumentID;
        ///交易所代码
        APINamespace TThostFtdcExchangeIDType	ExchangeID;
        ///合约在交易所的代码
        APINamespace TThostFtdcExchangeInstIDType	ExchangeInstID;
        ///最新价
        APINamespace TThostFtdcPriceType	LastPrice;
        ///上次结算价
        APINamespace TThostFtdcPriceType	PreSettlementPrice;
        ///昨收盘
        APINamespace TThostFtdcPriceType	PreClosePrice;
        ///昨持仓量
        APINamespace TThostFtdcLargeVolumeType	PreOpenInterest;
        ///今开盘
        APINamespace TThostFtdcPriceType	OpenPrice;
        ///最高价
        APINamespace TThostFtdcPriceType	HighestPrice;
        ///最低价
        APINamespace TThostFtdcPriceType	LowestPrice;
        ///数量
        APINamespace TThostFtdcVolumeType	Volume;
        ///成交金额
        APINamespace TThostFtdcMoneyType	Turnover;
        ///持仓量
        APINamespace TThostFtdcLargeVolumeType	OpenInterest;
        ///今收盘
        APINamespace TThostFtdcPriceType	ClosePrice;
        ///本次结算价
        APINamespace TThostFtdcPriceType	SettlementPrice;
        ///涨停板价
        APINamespace TThostFtdcPriceType	UpperLimitPrice;
        ///跌停板价
        APINamespace TThostFtdcPriceType	LowerLimitPrice;
        ///昨虚实度
        APINamespace TThostFtdcRatioType	PreDelta;
        ///今虚实度
        APINamespace TThostFtdcRatioType	CurrDelta;
        ///最后修改时间
        APINamespace TThostFtdcTimeType	UpdateTime;
        ///最后修改毫秒
        APINamespace TThostFtdcMillisecType	UpdateMillisec;
        ///申买价一
        APINamespace TThostFtdcPriceType	BidPrice1;
        ///申买量一
        APINamespace TThostFtdcVolumeType	BidVolume1;
        ///申卖价一
        APINamespace TThostFtdcPriceType	AskPrice1;
        ///申卖量一
        APINamespace TThostFtdcVolumeType	AskVolume1;
        ///申买价二
        APINamespace TThostFtdcPriceType	BidPrice2;
        ///申买量二
        APINamespace TThostFtdcVolumeType	BidVolume2;
        ///申卖价二
        APINamespace TThostFtdcPriceType	AskPrice2;
        ///申卖量二
        APINamespace TThostFtdcVolumeType	AskVolume2;
        ///申买价三
        APINamespace TThostFtdcPriceType	BidPrice3;
        ///申买量三
        APINamespace TThostFtdcVolumeType	BidVolume3;
        ///申卖价三
        APINamespace TThostFtdcPriceType	AskPrice3;
        ///申卖量三
        APINamespace TThostFtdcVolumeType	AskVolume3;
        ///申买价四
        APINamespace TThostFtdcPriceType	BidPrice4;
        ///申买量四
        APINamespace TThostFtdcVolumeType BidVolume4;
        ///申卖价四
        APINamespace TThostFtdcPriceType	 AskPrice4;
        ///申卖量四
        APINamespace TThostFtdcVolumeType	AskVolume4;
        ///申买价五
        APINamespace TThostFtdcPriceType	BidPrice5;
        ///申买量五
        APINamespace TThostFtdcVolumeType	BidVolume5;
        ///申卖价五
        APINamespace TThostFtdcPriceType	AskPrice5;
        ///申卖量五
        APINamespace TThostFtdcVolumeType	AskVolume5;
        ///当日均价
        APINamespace TThostFtdcPriceType	AveragePrice;
        ///业务日期
        APINamespace TThostFtdcDateType	ActionDay;
    };

    /// K线存储数据结构
    struct KMarketDataField
    {
        APINamespace TThostFtdcInstrumentIDType Contract;
        APINamespace TThostFtdcDateType TradingDay;
        /// K线周期标识60s=1m
        int SecondsPeriod;
        /// 当日第几根K线
        int BarSerials;
        /// 开
        double OpenPrice;
        /// 收
        double ClosePrice;
        /// 高
        double HighestPrice;
        /// 低
        double LowestPrice;
        /// 成交量
        double Volume;
        /// 成交额
        double Turnover;
        long begintime;
        long endtime;
    };

    /// 交易时间线
    struct TradingMinuteField
    {
        /// 分钟数索引
        int  CurrentMinute;
        APINamespace TThostFtdcDateType TradingDay;
        /// 是否交易时间
        bool isTradeFlag;
        /// 1分K线Bar序号
        int  BarSerials_1Min;
        /// 3分K线Bar序号
        int  BarSerials_3Min;
        /// 5分K线Bar序号
        int  BarSerials_5Min;
        /// 10分K线Bar序号
        int  BarSerials_10Min;
        /// 15分K线Bar序号
        int  BarSerials_15Min;
        /// 30分K线Bar序号
        int  BarSerials_30Min;
        /// 60分K线Bar序号
        int  BarSerials_60Min;
        /// 偏移量
        int  OffsetValue;
    };

    class MarketQuotationAPI_EXPORT MarketQuotationAPI : public APINamespace CThostFtdcMdSpi
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
        /// 链接行情服务器
        int initializeConnection(APINamespace TThostFtdcBrokerIDType, APINamespace TThostFtdcUserIDType, APINamespace TThostFtdcPasswordType, char*);
        /// 初始化日志文件
        int initializeLog();
    public:
        MarketQuotationAPI(void);
        MarketQuotationAPI(APINamespace TThostFtdcBrokerIDType, APINamespace TThostFtdcUserIDType, APINamespace TThostFtdcPasswordType, char*);
        ~MarketQuotationAPI(void);
#pragma endregion

        /*
        * 当前API接口状态
        * 接口运行变量
        */
#pragma region
    private:
        /// 请求id，递增变量
        int m_nRequestID;
        /// 记录收到数据量
        int m_nRecievedDataCount;
        /// 事件锁
        HANDLE m_hInitEvent;
        /// 行情接口请求
        APINamespace CThostFtdcMdApi *m_pUserApi;
        /// 接口状态
        APIStatus m_nAPIStatus;
        /// 设置接口状态
        void setAPIStatus(APIStatus);
    public:
        /// 获得接口运行状态
        APIStatus getAPIStatus();
#pragma endregion

        /*
        * 行情
        */
#pragma region
    private:
        /// 最新行情存储结构
        std::hash_map<std::string, struct MarketDataField> m_hashMarketDataList;
#ifdef KLINESTORAGE
        /// 1分钟K线存储结构,eg.订阅2个合约,存储2个合约1分钟K线的头地址
        std::vector<KMarketDataField*> m_array1mKLine;
        /// 3分钟K线存储结构,eg.订阅2个合约,存储2个合约3分钟K线的头地址
        std::vector<KMarketDataField*> m_array3mKLine;
        /// 5分钟K线存储结构,eg.订阅2个合约,存储2个合约5分钟K线的头地址
        std::vector<KMarketDataField*> m_array5mKLine;
        /// 10分钟K线存储结构,eg.订阅2个合约,存储2个合约10分钟K线的头地址
        std::vector<KMarketDataField*> m_array10mKLine;
        /// 15分钟K线存储结构,eg.订阅2个合约,存储2个合约15分钟K线的头地址
        std::vector<KMarketDataField*> m_array15mKLine;
        /// 30分钟K线存储结构,eg.订阅2个合约,存储2个合约30分钟K线的头地址
        std::vector<KMarketDataField*> m_array30mKLine;
        /// 60分钟K线存储结构,eg.订阅2个合约,存储2个合约60分钟K线的头地址
        std::vector<KMarketDataField*> m_array60mKLine;
        /// 合约序列
        std::vector<Contract> m_arrayContracts;
        /// 时间轴数据，记录交易时间与K线序列的对应
        TradingMinuteField **m_TradingTimeLine;
        /// 初始化K线时间线数据
        void initiateTradingTimeLine(char**, int);
        /// 订阅新合约初始化K线存储
        int initialKMarketDataSingle(const char*);
        /// 查找合约是否已订阅，并获取到合约数据索引号
        int findMarketDataIndex(const char*);
        /// (弃用)查找当前时间是否属于交易时间内以及时间轴上位置
        int findCurrentTradingTimeLineOffset();
        /// 查找当前时间是否属于指定合约交易时间内以及时间轴上位置
        int findCurrentTradingTimeLineOffset(const char*);
        /// 记录K线数据
        void recordKData(const char*);
#endif KLINESTORAGE
    public:
        /// 订阅多个行情
        void subMarketData(char *pInstrumentList[], int in_nInstrumentCount);
        /// 订阅一个行情
        int subMarketDataSingle(char *in_strInstrument);
        /// 得到指定合约的行情
        double getCurrPrice(const char *in_strInstrument);
        MarketDataField *getCurrentPrice(const char *in_strInstrument);
#ifdef KLINESTORAGE
        /// 得到指定合约的K线数据,指定与最新K线的偏移量,默认得到最新的1分钟K线数据
        KMarketDataField *getKLine(const char *in_strInstrument, int in_iSecondsPeriod = 1, int in_iCurrentOffset = 0);
        /// 得到指定合约的K线数据,指定所处当天K线的位置,默认得到第一分钟的1分钟K线数据
        KMarketDataField *getKLineBySerials(const char *in_strInstrument, int in_iSecondsPeriod = 1, int in_iPosition = 1);
#endif KLINESTORAGE
#pragma endregion

        /*
        * 数据落地
        */
#pragma region
    private:
#ifdef HQDATAFILE
        /// 实时行情写文件接口
        DataIntoFiles *m_pHQDataIntoFiles;
        /// 初始化实时行情写文件组件
        void initializeHQDataIntoFiles();
#endif HQDATAFILE
#ifdef KLINESTORAGE
#ifdef KDATAFILE
        /// KBar写文件接口
        DataIntoFiles *m_pKBarDataIntoFiles;
        /// 初始化KBar写文件组件
        void initializeKBarDataIntoFiles();
#endif KDATAFILE
        /// KBar落地
        void writeKBarData();
#endif KLINESTORAGE
        /// 实时行情落地
        void writeHQData(APINamespace CThostFtdcDepthMarketDataField *);
#pragma endregion

        /*
        * 交易所时间
        */
#pragma region
    private:
        /*
        * 与交易所时间差
        */
        double m_nDCETimeDiff;
        double m_nCZCETimeDiff;
        double m_nSHFETimeDiff;
        double m_nCFFEXTimeDiff;
        double m_nIMETimeDiff;
        /// 通过交易所名字获取到交易所时间（本地时间+/-交易所时间差）
        double getExchangeTime(char*);
        /// 获取本地时间
        double getLocalTime();
        /// 通过时间格式字符串(hh24:mi:ss)获取时间
        double getTimebyFormat(char*);
    public:
        /// 判断传入时间是否大于指定交易所时间
        bool overTime(char*, tm*);
#pragma endregion

        /*
        * 柜台接口提供
        */
#pragma region
    private:
        /// 当客户端与交易后台建立起通信连接时（还未登录前），该方法被调用。
        virtual void OnFrontConnected();
        ///当客户端与交易后台通信连接断开时，该方法被调用。当发生这个情况后，API会自动重新连接，客户端可不做处理。
        ///@param nReason 错误原因
        ///        0x1001 网络读失败
        ///        0x1002 网络写失败
        ///        0x2001 接收心跳超时
        ///        0x2002 发送心跳失败
        ///        0x2003 收到错误报文
        ///        0x2004 服务器主动断开
        virtual void OnFrontDisconnected(int nReason);
        /// 心跳超时警告。当长时间未收到报文时，该方法被调用。
        /// @param nTimeLapse 距离上次接收报文的时间
        virtual void OnHeartBeatWarning(int nTimeLapse);
        /// 登录请求响应
        virtual void OnRspUserLogin(APINamespace CThostFtdcRspUserLoginField *pRspUserLogin, APINamespace CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
        /// 登出请求响应
        virtual void OnRspUserLogout(APINamespace CThostFtdcUserLogoutField *pUserLogout, APINamespace CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
        /// 错误应答
        virtual void OnRspError(APINamespace CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
        /// 订阅行情应答
        virtual void OnRspSubMarketData(APINamespace CThostFtdcSpecificInstrumentField *pSpecificInstrument, APINamespace CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
        /// 取消订阅行情应答
        virtual void OnRspUnSubMarketData(APINamespace CThostFtdcSpecificInstrumentField *pSpecificInstrument, APINamespace CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
        /// 深度行情通知
        virtual void OnRtnDepthMarketData(APINamespace CThostFtdcDepthMarketDataField *pDepthMarketData);
    public:
#pragma endregion
    };
}

#endif _MARKETQUOTATIONAPI_H_