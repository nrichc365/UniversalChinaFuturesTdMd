#define MQI_RELATIVESTRENGTHINDEX_EXE
#include "MQI_RelativeStrengthIndex.h"
#include <iostream>
#include <thread>

axapi::MQI_RelativeStrengthIndex::MQI_RelativeStrengthIndex()
{
    char* t_strLogFuncName = "MQI_RelativeStrengthIndex::MQI_RelativeStrengthIndex";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    //LOG4CPLUS_TRACE(m_objLogger, t_strLog);
}

axapi::MQI_RelativeStrengthIndex::~MQI_RelativeStrengthIndex()
{
    char* t_strLogFuncName = "MQI_RelativeStrengthIndex::MQI_RelativeStrengthIndex";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    //LOG4CPLUS_TRACE(m_objLogger, t_strLog);
}

void axapi::MQI_RelativeStrengthIndex::caculate()
{
    char* t_strLogFuncName = "MQI_RelativeStrengthIndex::caculate";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    //LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    std::string t_strContract = m_strContract;

#pragma region
    /*
    * 自定义指标计算部分
    */
    double t_ClosePriceSUM_Ascend = 0;
    double t_ClosePriceSUM_Descend = 0;
    double t_ClosePrice = 0;
    MQI_RelativeStrengthIndexField t_objLatestIndexValue;
    t_objLatestIndexValue.BarSerials = NULL;
    t_objLatestIndexValue.indexValue = NULL;

    axapi::KMarketDataField *t_pMarketDataField = NULL;
    axapi::KMarketDataField *t_pMarketDataFieldLastest = NULL;

    while (m_blAutoRun)
    {
        sprintf_s(t_strLog, 500, "%s:RSI caculating%s...", t_strLogFuncName, t_strContract.c_str());
        //LOG4CPLUS_TRACE(m_objLogger, t_strLog);

        t_pMarketDataFieldLastest = m_pMarketQuotation->getKLine(t_strContract.c_str(), 1, 0);
        if (t_pMarketDataFieldLastest == NULL)
        {
            continue;
        }
        for (int i = 0; i < m_n1mKBars; i++)
        {
            t_pMarketDataField = m_pMarketQuotation->getKLineBySerials(t_strContract.c_str(), 1, t_pMarketDataFieldLastest->BarSerials - i);
            if (t_pMarketDataField != NULL)
            {
                if (t_pMarketDataField->ClosePrice - t_pMarketDataField->OpenPrice >= 0)
                    t_ClosePriceSUM_Ascend += t_pMarketDataField->ClosePrice - t_pMarketDataField->OpenPrice;
                else
                    t_ClosePriceSUM_Descend += t_pMarketDataField->OpenPrice - t_pMarketDataField->ClosePrice;
            }
            else
            {
                break;
            }
        }
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
            if (t_ClosePriceSUM_Ascend + t_ClosePriceSUM_Descend == 0)
            {
                t_objLatestIndexValue.BarSerials = t_pMarketDataFieldLastest->BarSerials;
                t_objLatestIndexValue.indexValue = 0;
                m_arrayIndexValue.push_back(t_objLatestIndexValue);
            }
            else
            {
                t_objLatestIndexValue.BarSerials = t_pMarketDataFieldLastest->BarSerials;
                t_objLatestIndexValue.indexValue = t_ClosePriceSUM_Ascend / (t_ClosePriceSUM_Ascend + t_ClosePriceSUM_Descend) * 100;
                m_arrayIndexValue.push_back(t_objLatestIndexValue);
            }
        }
#pragma endregion

        Sleep(100);
    }

    SetEvent(m_hCaculateRuning);
    sprintf_s(t_strLog, 500, "%s:RSI exit caculate%s", t_strLogFuncName, t_strContract.c_str());
    //LOG4CPLUS_TRACE(m_objLogger, t_strLog);
}

double axapi::MQI_RelativeStrengthIndex::getIndexValue(int in_iCurrentOffset)
{
    char* t_strLogFuncName = "MQI_RelativeStrengthIndex::getIndexValue";
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