#ifndef _MQI_MACDINDEX_H_
#define _MQI_MACDINDEX_H_
#pragma once

#ifdef MQI_MACDINDEX_EXE
#define MQI_MACDINDEX_EXPORT
#else
#ifdef MQI_MACDINDEX_EXP
#define MQI_MACDINDEX_EXPORT __declspec(dllexport)
#else
#define MQI_MACDINDEX_EXPORT __declspec(dllimport)
#endif MQI_MACDINDEX_EXP
#endif MQI_MACDINDEX_EXE

#include "MarketQuotationIndex.h"

/*
* Moving Average Convergence / Divergence
以EMA1的参数为12日EMA2的参数为26日，DIF的参数为9日为例来看看MACD的计算过程
1、计算移动平均值（EMA）
12日EMA的算式为
EMA（12）=前一日EMA（12）×11/13+今日收盘价×2/13
26日EMA的算式为
EMA（26）=前一日EMA（26）×25/27+今日收盘价×2/27
2、计算离差值（DIF）
DIF=今日EMA（12）－今日EMA（26）
3、计算DIF的9日EMA
根据离差值计算其9日的EMA，即离差平均值，是所求的MACD值。为了不与指标原名相混淆，此值又名
DEA或DEM。
今日DEA（MACD）=前一日DEA×8/10+今日DIF×2/10。
计算出的DIF和DEA的数值均为正值或负值。
用（DIF-DEA）×2即为MACD柱状图。
*/
namespace axapi
{
    /// RSI指标结构
    struct MQI_MACDIndexField
    {
        /// 指标值
        double indexValue;
        /// 当日第几根K线
        int BarSerials;
        /// EMA1
        double EMA1Value;
        /// EMA2
        double EMA2Value;
        /// DIF
        double DIFValue;
        /// DEA DEM
        double DEAValue;
    };

    class MQI_MACDIndex :
        public MarketQuotationIndex
    {
    public:
        MQI_MACDIndex();
        ~MQI_MACDIndex();
        int initialize(axapi::MarketQuotationAPI*, unsigned int, unsigned int, unsigned int, std::string);
        /// 获得指定位置的指标值 错误返回NULL
        double getIndexValue(int in_iCurrentOffset = 0);
    private:
        int m_n1mKBars4EMA1;
        int m_n1mKBars4EMA2;
        int m_n1mKBars4DEA;
        /// 指标序列
        std::vector<MQI_MACDIndexField> m_arrayIndexValue;
        /// 计算入口 重新定义为RSI
        void caculate();
    };
}

#endif _MQI_MACDINDEX_H_