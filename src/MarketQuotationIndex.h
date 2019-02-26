#ifndef _RELATIVESTRENGTHINDEX_H_
#define _RELATIVESTRENGTHINDEX_H_
#pragma once

#ifdef RELATIVESTRENGTHINDEX_EXE
#define RELATIVESTRENGTHINDEX_EXPORT
#else
#ifdef RELATIVESTRENGTHINDEX_EXP
#define RELATIVESTRENGTHINDEX_EXPORT __declspec(dllexport)
#else
#define RELATIVESTRENGTHINDEX_EXPORT __declspec(dllimport)
#endif RELATIVESTRENGTHINDEX_EXP
#endif RELATIVESTRENGTHINDEX_EXE

#define CTP_TRADEAPI
#define KLINESTORAGE
#include <MarketQuotationAPI.h>
#include <string>
#include <vector>

namespace axapi
{
    class RELATIVESTRENGTHINDEX_EXPORT MarketQuotationIndex
    {
    public:
        /// 获得指定位置的指标值 错误返回NULL
        double getIndexValue(int in_iCurrentOffset = 0);
        /// 外接行情数据初始化，调用后开始计算指标值
        int initialize(axapi::MarketQuotationAPI*, unsigned int, std::string);
        MarketQuotationIndex(void);
        ~MarketQuotationIndex(void);

    private:
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

#endif _RELATIVESTRENGTHINDEX_H_