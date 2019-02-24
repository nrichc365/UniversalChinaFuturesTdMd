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
#include <vector>

namespace axapi
{
    class MarketQuotationIndex
    {
    public:

        /// index计算入口
        void caculate();
        double getIndexValue();
        /// 外接行情数据
        MarketQuotationIndex(axapi::MarketQuotationAPI*, int);
        void initialize(axapi::MarketQuotationAPI*, int);
        MarketQuotationIndex(void);
        ~MarketQuotationIndex(void);

    private:
        /// 外接行情数据
        axapi::MarketQuotationAPI* m_pMarketQuotation;
        /// 1分钟K线计算的指标 m_array1mIndex每个成员记录不同合约index数据的起始值地址
        std::vector<double*> m_array1mIndex;
        /// 合约序列
        std::vector<char*> m_arrayContracts;
        /// 指标参数
        int m_n1mKBars;
        /// 指标值
        double m_nIndexValue;

    };
}

#endif _RELATIVESTRENGTHINDEX_H_