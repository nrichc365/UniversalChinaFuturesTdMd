#ifndef _MARKETQUOTATIONAPI_HC_DEFINATION_H_
#define _MARKETQUOTATIONAPI_HC_DEFINATION_H_
#pragma once

namespace axapi
{
    /// 仿真行情时钟线
    struct TradingClockField
    {
        /// 自然日
        char basedate[9];
        /// 时间hh:mm:ss
        char minute[9];
        /// 毫秒
        int millisecond;
        /// 序号
        int sequence;
        /// 是否交易标志
        char istradeflag;
    };
}
#endif _MARKETQUOTATIONAPI_HC_DEFINATION_H_