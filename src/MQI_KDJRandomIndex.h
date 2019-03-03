#ifndef _MQI_KDJRANDOMINDEX_H_
#define _MQI_KDJRANDOMINDEX_H_
#pragma once

#ifdef MQI_KDJRANDOMINDEX_EXE
#define MQI_KDJRANDOMINDEX_EXPORT
#else
#ifdef MQI_KDJRANDOMINDEX_EXP
#define MQI_KDJRANDOMINDEX_EXPORT __declspec(dllexport)
#else
#define MQI_KDJRANDOMINDEX_EXPORT __declspec(dllimport)
#endif MQI_KDJRANDOMINDEX_EXP
#endif MQI_KDJRANDOMINDEX_EXE

#include "MarketQuotationIndex.h"

/*
* KDJ的计算比较复杂，首先要计算周期（n日、n周等）的RSV值，即未成熟随机指标值，然后再计算K值、D值、J值等。以n日KDJ数值的计算为例，其计算公式为
n日RSV=（Cn－Ln）/（Hn－Ln）×100
公式中，Cn为第n日收盘价；Ln为n日内的最低价；Hn为n日内的最高价。
其次，计算K值与D值：
当日K值=2/3×前一日K值+1/3×当日RSV
当日D值=2/3×前一日D值+1/3×当日K值
若无前一日K 值与D值，则可分别用50来代替。
J值=3*当日K值-2*当日D值
以9日为周期的KD线为例，即未成熟随机值，计算公式为
9日RSV=（C－L9）÷（H9－L9）×100
公式中，C为第9日的收盘价；L9为9日内的最低价；H9为9日内的最高价。
K值=2/3×第8日K值+1/3×第9日RSV
D值=2/3×第8日D值+1/3×第9日K值
J值=3*第9日K值-2*第9日D值
若无前一日K
值与D值，则可以分别用50代替。
*/

namespace axapi
{
    /// RSI指标结构
    struct MQI_KDJRandomIndexField
    {
        /// 指标值
        double indexValue;
        /// 当日第几根K线
        int BarSerials;
        /// RSV值
        int RSVValue;
        /// K
        double KValue;
        /// D
        double DValue;
        /// J
        double JValue;
    };

    class MQI_KDJRandomIndex :
        public MarketQuotationIndex
    {
    public:
        MQI_KDJRandomIndex();
        ~MQI_KDJRandomIndex();
        /// 获得指定位置的指标值 错误返回NULL
        double getIndexValue(int in_iCurrentOffset = 0);
    private:
        /// 指标序列
        std::vector<MQI_KDJRandomIndexField> m_arrayIndexValue;
        /// 计算入口 重新定义为RSI
        void caculate();
    };
}

#endif _MQI_KDJRANDOMINDEX_H_