#define MarketQuotationAPI_HC_EXE
#define DBDataSource
#define OTL_ORA10G
//#define TimingProcess
#include "MarketQuotationAPI_HC.h"
#include "ClockSimulated.h"
#define TestRound 80000*40

/// 带行情读取下最快模拟速度测试
void test_order(axapi::ClockSimulated *t_clock, axapi::MarketQuotationAPI_HC *t_mq)
{
    long t_count = 0;
    int t_lastsequence = 0;
    std::string t_info = "";
    int sequence = 0;

    while (t_count < TestRound)
    {
        t_mq->getCurrentPrice("ag1912", &sequence);
#pragma region
        /*策略部分*/

#pragma endregion
        t_count++;
    }
    std::cout << "未连续读取行情(上一次行情序号：本次行情序号)：" << t_info.c_str() << std::endl;
    std::cout << "最后读取为第" << t_lastsequence << "条数据" << std::endl;
}

/// 带行情读取下最快模拟速度测试
void test_mqread(axapi::ClockSimulated *t_clock, axapi::MarketQuotationAPI_HC *t_mq)
{
    long t_count = 0;
    int t_lastsequence = 0;
    std::string t_info = "";
    int sequence = 0;

    while (t_count < TestRound)
    {
        t_mq->getCurrentPrice("ag1912", &sequence);
        if (sequence - t_lastsequence > 1/* || sequence - t_lastsequence == 0*/)
        {
            //std::cout << "error";
            t_info += std::to_string(sequence);
            t_info += ":";
            t_info += std::to_string(t_lastsequence);
            t_info += ";";
        }
        if (sequence > TestRound)
        {
            std::cout << "over size:" << t_count << std::endl;
            break;
        }
        t_lastsequence = sequence;
        t_count++;
    }
    std::cout << "未连续读取行情(上一次行情序号：本次行情序号)：" << t_info.c_str() << std::endl;
    std::cout << "最后读取为第" << t_lastsequence << "条数据" << std::endl;
}

/// 无行情请求下最快模拟速度测试
void test_clock(axapi::ClockSimulated *t_clock)
{
    long t_count = 0;
    int t_lastsequence = 0;
    std::string t_info = "";
    int sequence;

    while (t_count < TestRound)
    {
        sequence = t_clock->getCurrentTime()->sequence;
        if (sequence - t_lastsequence > 1/* || sequence - t_lastsequence == 0*/)
        {
            //std::cout << "error";
            t_info += std::to_string(sequence);
            t_info += ":";
            t_info += std::to_string(t_lastsequence);
            t_info += ";";
        }
        if (sequence > TestRound)
        {
            std::cout << "over size" << std::endl;
        }
        t_lastsequence = sequence;
        t_count++;
    }
    std::cout << "未连续读取行情(上一次行情序号：本次行情序号)：" << t_info.c_str() << std::endl;
    std::cout << "最后读取为第" << t_lastsequence << "条数据" << std::endl;
}

/// 测试high_resolution_clock
void test()
{
    long t_count = 0;
    auto t_time = std::chrono::high_resolution_clock::now();
    auto t_time2 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> t_duration;
    double t_dbduration, t_maxDuration = 0, t_sumDuration = 0;
    /*for (int i = 0; i > 100000; i++)
    {
    t_time = t_time2;
    }*/
    while (t_count++ < TestRound)
    {
        t_time2 = std::chrono::high_resolution_clock::now();
        t_duration = t_time2 - t_time;
        t_dbduration = t_duration.count();
        t_maxDuration = t_maxDuration < t_dbduration ? t_dbduration : t_maxDuration;
        t_sumDuration += t_dbduration;
        std::cout << "用时" << t_duration.count() << "秒" << std::endl;
        t_time = t_time2;
        Sleep(1000);
    }
    std::cout << "    共用时" << t_sumDuration << "秒" << std::endl;
    std::cout << "单次均用时" << t_sumDuration / t_count << "秒" << std::endl;
    std::cout << "  最大用时" << t_maxDuration << "秒" << std::endl;
}

void main()
{
    axapi::ClockSimulated *t_clock = new axapi::ClockSimulated();
    axapi::MarketQuotationAPI_HC *t_mq = new axapi::MarketQuotationAPI_HC();
    
    t_clock->setSpeedMultiplier(1);
    t_mq->initialClockSimulated(t_clock);
    t_mq->subMarketDataSingle("ag1912", "20190904");
    t_clock->startClocking();

    auto t_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> t_duration;

    //test_clock(t_clock);
    test_mqread(t_clock, t_mq);

    auto t_time2 = std::chrono::high_resolution_clock::now();
    t_duration = t_time2 - t_time;
    std::cout << "测试共用时" << t_duration.count() << "毫秒" << std::endl;

}