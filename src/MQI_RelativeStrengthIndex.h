#ifndef _MQI_RELATIVESTRENGTHINDEX_H_
#define _MQI_RELATIVESTRENGTHINDEX_H_
#pragma once

#ifdef MQI_RELATIVESTRENGTHINDEX_EXE
#define MQI_RELATIVESTRENGTHINDEX_EXPORT
#else
#ifdef MQI_RELATIVESTRENGTHINDEX_EXP
#define MQI_RELATIVESTRENGTHINDEX_EXPORT __declspec(dllexport)
#else
#define MQI_RELATIVESTRENGTHINDEX_EXPORT __declspec(dllimport)
#endif MQI_RELATIVESTRENGTHINDEX_EXP
#endif MQI_RELATIVESTRENGTHINDEX_EXE

#include "MarketQuotationIndex.h"

namespace axapi
{
    /// RSI指标结构
    struct MQI_RelativeStrengthIndexField
    {
        /// 指标值
        double indexValue;
        /// 当日第几根K线
        int BarSerials;
    };

    class MQI_RelativeStrengthIndex :
        public MarketQuotationIndex
    {
    public:
        MQI_RelativeStrengthIndex();
        ~MQI_RelativeStrengthIndex();
        /// 获得指定位置的指标值 错误返回NULL
        double getIndexValue(int in_iCurrentOffset = 0);
    private:
        /// 指标序列
        std::vector<MQI_RelativeStrengthIndexField> m_arrayIndexValue;
        /// 计算入口 重新定义为RSI
        void caculate();
    };
}

#endif _MQI_RELATIVESTRENGTHINDEX_H_