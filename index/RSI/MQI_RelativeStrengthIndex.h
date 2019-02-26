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
    class MQI_RelativeStrengthIndex :
        public MarketQuotationIndex
    {
    public:
        MQI_RelativeStrengthIndex();
        ~MQI_RelativeStrengthIndex();
        int initialize(axapi::MarketQuotationAPI *, unsigned int, std::string);
    private:
        /// 计算入口 重新定义为RSI
        void caculate();
    };
}

#endif _MQI_RELATIVESTRENGTHINDEX_H_