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

namespace axapi
{
    /// RSI指标结构
    struct MQI_KDJRandomIndexField
    {
        /// 指标值
        double indexValue;
        /// 当日第几根K线
        int BarSerials;
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