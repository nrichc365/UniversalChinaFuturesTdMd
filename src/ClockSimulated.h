#ifndef _CLOCKSIMULATED_H_
#define _CLOCKSIMULATED_H_
#pragma once

#define _MQREAD_MODE_
#include <chrono>
#include <iostream>
#include <vector>
#include "MarketQuotationAPI_HC_Defination.h"

namespace axapi
{
    /// 仿真行情时钟线
    struct ClockSimulatedField
    {
        /// 自然日
        char basedate[9];
        /// 时间hh:mm:ss
        char minute[9];
        /// 毫秒
        int millisecond;
        /// 序号
        int sequence;
    };

    class ClockSimulated
    {
    private:
        /// 存放模拟时间数据集
        std::vector<ClockSimulatedField> m_vClockSimulatedList;
        /// 存放开始运行的时间
        std::chrono::high_resolution_clock::time_point m_starttime;
        /// 速度乘数 设置为10^9表示未加速，10^6标识加速1000倍，模拟时钟运行1000秒等于现实1秒，可模拟时间最快为_MIN_SPEEDMULTIPLIER
        long m_nSpeedMultiplier;
    public:
        ClockSimulated();
        ~ClockSimulated();
        /// 设置速度乘数，乘数越大，模拟越慢 10^9：实际1秒=模拟1秒 10^6：实际1毫秒=模拟1秒 10^3：实际1微秒=模拟1秒 10^0：实际1纳秒=模拟1秒
        long setSpeedMultiplier(long);
        /// 填充模拟时钟的区间时间线 比如要模拟20190903 08:00:00-21:00:00区间内按秒模拟，则时间线应该包含区间内每秒的时间
        void fillupClockLine(std::vector<TradingClockField>*);
        /// 开始计时
        void startClocking();
        /// 获得当前时间
        ClockSimulatedField *getCurrentTime();
    };
}

#endif _CLOCKSIMULATED_H_