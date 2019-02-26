#ifndef _MARKETQUOTATIONINDEX_H_
#define _MARKETQUOTATIONINDEX_H_
#pragma once

#ifdef MARKETQUOTATIONINDEX_EXE
#define MARKETQUOTATIONINDEX_EXPORT
#else
#ifdef MARKETQUOTATIONINDEX_EXP
#define MARKETQUOTATIONINDEX_EXPORT __declspec(dllexport)
#else
#define MARKETQUOTATIONINDEX_EXPORT __declspec(dllimport)
#endif MARKETQUOTATIONINDEX_EXP
#endif MARKETQUOTATIONINDEX_EXE

#define _SCL_SECURE_NO_WARNINGS
#define CTP_TRADEAPI
#define KLINESTORAGE
#include <MarketQuotationAPI.h>
#include <string>
#include <vector>

namespace axapi
{
    class MARKETQUOTATIONINDEX_EXPORT MarketQuotationIndex
    {
    public:
        /// 获得指定位置的指标值 错误返回NULL
        double getIndexValue(int in_iCurrentOffset = 0);
        /// 外接行情数据初始化，调用后开始计算指标值
        int initialize(axapi::MarketQuotationAPI*, unsigned int, std::string);
        MarketQuotationIndex(void);
        ~MarketQuotationIndex(void);

    protected:
        /// 外接行情数据
        axapi::MarketQuotationAPI* m_pMarketQuotation;
        /// 当前指标合约
        std::string m_strContract;
        /// 指标参数 一个指标使用几根1m行情KBar
        unsigned int m_n1mKBars;
        /// 指标序列
        std::vector<double> m_arrayIndexValue;
        /// 计算入口 继承后需用户自己定义
        void caculate();
        /// 计算停止标志
        bool m_blAutoRun;
        HANDLE m_hCaculateRuning;
    };
}

#endif _MARKETQUOTATIONINDEX_H_