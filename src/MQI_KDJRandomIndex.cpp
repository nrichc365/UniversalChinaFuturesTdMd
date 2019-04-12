#define MQI_KDJRandomIndex_EXE
#include "MQI_KDJRandomIndex.h"
#include <iostream>
#include <thread>

axapi::MQI_KDJRandomIndex::MQI_KDJRandomIndex()
{
    char* t_strLogFuncName = "MQI_KDJRandomIndex::MQI_KDJRandomIndex";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    //LOG4CPLUS_TRACE(m_objLogger, t_strLog);
}

axapi::MQI_KDJRandomIndex::~MQI_KDJRandomIndex()
{
    char* t_strLogFuncName = "MQI_KDJRandomIndex::MQI_KDJRandomIndex";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    //LOG4CPLUS_TRACE(m_objLogger, t_strLog);
}

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
void axapi::MQI_KDJRandomIndex::caculate()
{
    char* t_strLogFuncName = "MQI_KDJRandomIndex::caculate";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    //LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    std::string t_strContract = m_strContract;

#pragma region
    /*
    * 自定义指标计算部分
    */
    // 区间收盘价Cn, 区间最低价Ln, 区间最高价Hn
    double t_ClosePrice, t_LowestPrice, t_HighestPrice;
    MQI_KDJRandomIndexField t_objLatestIndexValue;
    t_objLatestIndexValue.BarSerials = NULL;
    t_objLatestIndexValue.indexValue = NULL;

    axapi::KMarketDataField *t_pMarketDataField = NULL;
    axapi::KMarketDataField *t_pMarketDataFieldLastest = NULL;

    while (m_blAutoRun)
    {
        sprintf_s(t_strLog, 500, "%s:KDJ caculating%s...", t_strLogFuncName, t_strContract.c_str());
        //LOG4CPLUS_TRACE(m_objLogger, t_strLog);

        // 区间收盘价Cn
        t_ClosePrice = 0;
        // 区间最低价Ln
        t_LowestPrice = 9999999;
        // 区间最高价Hn
        t_HighestPrice = 0;

        t_pMarketDataFieldLastest = m_pMarketQuotation->getKLine(t_strContract.c_str(), 1, 0);
        if (t_pMarketDataFieldLastest == NULL)
        {
            continue;
        }

        /*
        * 寻找指标参数
        */
        for (int i = 0; i < m_n1mKBars; i++)
        {
            t_pMarketDataField = m_pMarketQuotation->getKLineBySerials(t_strContract.c_str(), 1, t_pMarketDataFieldLastest->BarSerials - i);
            if (t_pMarketDataField != NULL)
            {
                if (i == 0)
                {
                    t_ClosePrice = t_pMarketDataField->ClosePrice;
                }
                t_LowestPrice = t_LowestPrice > t_pMarketDataField->LowestPrice ? t_pMarketDataField->LowestPrice : t_LowestPrice;
                t_HighestPrice = t_HighestPrice < t_pMarketDataField->HighestPrice ? t_pMarketDataField->HighestPrice : t_HighestPrice;
            }
            else
            {
                break;
            }
        }

        /*
        * 计算指标
        */
        if (t_pMarketDataField == NULL)
        {
            continue;
        }
        else
        {
            /*
            * 记录INDEX值
            */
            /// 如果当前计算的INDEX值的序号未变则更新
            if (t_objLatestIndexValue.BarSerials == t_pMarketDataFieldLastest->BarSerials)
            {
                m_arrayIndexValue.pop_back();
            }

            t_objLatestIndexValue.BarSerials = t_pMarketDataFieldLastest->BarSerials;
            t_objLatestIndexValue.RSVValue = (t_HighestPrice - t_LowestPrice) == 0 ? 100 : (t_ClosePrice - t_LowestPrice) / (t_HighestPrice - t_LowestPrice) * 100;
            if (m_arrayIndexValue.size() > 0 && m_arrayIndexValue[m_arrayIndexValue.size() - 1].BarSerials == t_pMarketDataFieldLastest->BarSerials - 1)
            {
                t_objLatestIndexValue.KValue = m_arrayIndexValue[m_arrayIndexValue.size() - 1].KValue * 2 / 3 + t_objLatestIndexValue.RSVValue * 1 / 3;
                t_objLatestIndexValue.DValue = m_arrayIndexValue[m_arrayIndexValue.size() - 1].DValue * 2 / 3 + t_objLatestIndexValue.KValue * 1 / 3;
            }
            else
            {
                t_objLatestIndexValue.KValue = 50 + t_objLatestIndexValue.RSVValue * 1 / 3;
                t_objLatestIndexValue.DValue = 50 + t_objLatestIndexValue.KValue * 1 / 3;
            }
            t_objLatestIndexValue.JValue = t_objLatestIndexValue.KValue * 3 + t_objLatestIndexValue.DValue * 2;
            t_objLatestIndexValue.indexValue = t_objLatestIndexValue.JValue;
            m_arrayIndexValue.push_back(t_objLatestIndexValue);
        }
#pragma endregion

        Sleep(100);
    }

    SetEvent(m_hCaculateRuning);
    sprintf_s(t_strLog, 500, "%s:KDJ exit caculate%s", t_strLogFuncName, t_strContract.c_str());
    //LOG4CPLUS_TRACE(m_objLogger, t_strLog);
}

double axapi::MQI_KDJRandomIndex::getIndexValue(int in_iCurrentOffset)
{
    char* t_strLogFuncName = "MQI_KDJRandomIndex::getIndexValue";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    //LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    /*
    * 请求索引位置超出指标序列范围，则返回NULL
    */
    if (m_arrayIndexValue.size() + in_iCurrentOffset <= 0 || in_iCurrentOffset > 0)
    {
        return NULL;
    }

    /*
    * 返回指定位置的指标值
    */
    return m_arrayIndexValue[m_arrayIndexValue.size() + in_iCurrentOffset - 1].indexValue;
}