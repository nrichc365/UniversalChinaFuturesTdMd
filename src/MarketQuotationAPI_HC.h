#ifndef _MARKETQUOTATIONAPI_HC_H_
#define _MARKETQUOTATIONAPI_HC_H_
#pragma once

#define _SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#ifdef MarketQuotationAPI_HC_EXE
#define MarketQuotationAPI_HC_EXPORT
#else
#ifdef MarketQuotationAPI_HC_EXP
#define MarketQuotationAPI_HC_EXPORT __declspec(dllexport)
#else
#define MarketQuotationAPI_HC_EXPORT __declspec(dllimport)
#endif MarketQuotationAPI_HC_EXP
#endif MarketQuotationAPI_HC_EXE

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
#include <string>
#ifdef DBDataSource
#include <otlv4.h>
#endif DBDataSource

#include <CTPTdMd_API_common/ThostFtdcMdApi.h>
#include "MarketQuotationAPI_HC_Defination.h"
#include "ClockSimulated.h"
#define  APINamespace

#ifdef HQDATAFILE
#define DATAFILE
#endif HQDATAFILE
#ifdef KDATAFILE
#define DATAFILE
#endif KDATAFILE

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

    class MarketQuotationAPI_HC_EXPORT MarketQuotationAPI_HC : public APINamespace CThostFtdcMdSpi
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
#ifdef DBDataSource
        /// 数据库连接
        otl_connect m_DBConnector;
#endif DBDataSource
        /// 账号信息 经纪商ID
        ///APINamespace TThostFtdcBrokerIDType m_strBrokerID;
        /// 账号信息 客户号
        ///APINamespace TThostFtdcUserIDType m_strUserID;
        /// 账号信息 客户密码
        ///APINamespace TThostFtdcPasswordType m_strPassword;
        /// 链接行情服务器
        ///int initializeConnection(APINamespace TThostFtdcBrokerIDType, APINamespace TThostFtdcUserIDType, APINamespace TThostFtdcPasswordType, char*);
        /// 初始化日志文件
        int initializeLog();
    public:
        MarketQuotationAPI_HC(void);
        //MarketQuotationAPI_HC(APINamespace TThostFtdcBrokerIDType, APINamespace TThostFtdcUserIDType, APINamespace TThostFtdcPasswordType, char*);
        ~MarketQuotationAPI_HC(void);
#pragma endregion

        /*
        * 行情
        */
#pragma region
    private:
        /// 模拟时钟
        axapi::ClockSimulated *m_pClockSimulated;
        /// 最新行情存储结构
        std::hash_map<std::string, struct MarketDataField> m_hashMarketDataList;
        std::vector<struct MarketDataField> m_vMarketData;
        std::vector<TradingClockField> m_vTradingClock;
        /// 装载数据库中指定合约的行情数据
        int loadMarketData(const char*, const char*);
        /// 装载时间轴
        int loadTimeline();
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
        void initiateTradingTimeLine(char**, int) {};
        /// 订阅新合约初始化K线存储
        int initialKMarketDataSingle(const char*) {};
        /// 查找合约是否已订阅，并获取到合约数据索引号
        int findMarketDataIndex(const char*) {};
        /// (弃用)查找当前时间是否属于交易时间内以及时间轴上位置
        int findCurrentTradingTimeLineOffset() {};
        /// 查找当前时间是否属于指定合约交易时间内以及时间轴上位置
        int findCurrentTradingTimeLineOffset(const char*) {};
        /// 记录K线数据
        void recordKData(const char*) {};
#endif KLINESTORAGE
    public:
        /// 订阅多个行情
        void subMarketData(char *pInstrumentList[], int in_nInstrumentCount, char* pTradingday);
        /// 订阅一个行情
        int subMarketDataSingle(char *in_strInstrument, char *pTradingday);
        /// 得到指定合约的行情
        double getCurrPrice(const char *in_strInstrument) {};
        MarketDataField *getCurrentPrice(const char *in_strInstrument);
        /// 用于测试最大模拟速度 第二个参数用于回写模拟行情的序列号
        MarketDataField *getCurrentPrice(const char *in_strInstrument, int *);
        /// 初始化模拟时钟,用已经加载好的时间轴数据初始化外部的时钟
        int initialClockSimulated(ClockSimulated*);
#ifdef KLINESTORAGE
        /// 得到指定合约的K线数据,指定与最新K线的偏移量,默认得到最新的1分钟K线数据
        KMarketDataField *getKLine(const char *in_strInstrument, int in_iSecondsPeriod = 1, int in_iCurrentOffset = 0) {};
        /// 得到指定合约的K线数据,指定所处当天K线的位置,默认得到第一分钟的1分钟K线数据
        KMarketDataField *getKLineBySerials(const char *in_strInstrument, int in_iSecondsPeriod = 1, int in_iPosition = 1) {};
#endif KLINESTORAGE
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
        double getExchangeTime(char*) {};
        /// 获取本地时间
        double getLocalTime() {};
        /// 通过时间格式字符串(hh24:mi:ss)获取时间
        double getTimebyFormat(char*) {};
    public:
        /// 判断传入时间是否大于指定交易所时间
        bool overTime(char*, tm*) {};
#pragma endregion

    };
}

#endif _MARKETQUOTATIONAPI_HC_H_