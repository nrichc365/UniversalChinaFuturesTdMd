#define MQI_MACDINDEX_EXE
#include "MQI_MACDIndex.h"
#include <iostream>
#include <thread>

axapi::MQI_MACDIndex::MQI_MACDIndex()
{
    char* t_strLogFuncName = "MQI_MACDIndex::MQI_MACDIndex";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    //LOG4CPLUS_TRACE(m_objLogger, t_strLog);
}

axapi::MQI_MACDIndex::~MQI_MACDIndex()
{
    char* t_strLogFuncName = "MQI_MACDIndex::MQI_MACDIndex";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    //LOG4CPLUS_TRACE(m_objLogger, t_strLog);
}

int axapi::MQI_MACDIndex::initialize(axapi::MarketQuotationAPI *in_pMarketQuotationAPI,
    unsigned int in_n1mKBars4EMA1, unsigned int in_n1mKBars4EMA2, unsigned int in_n1mKBars4DEA, std::string in_strContract)
{
    char* t_strLogFuncName = "MarketQuotationIndex::initialize";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    //LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    if (m_blAutoRun == true)
    {
        m_blAutoRun = false;
        WaitForSingleObject(m_hCaculateRuning, INFINITE);
    }
    /// 传入参数错误则返回错误
    if (in_pMarketQuotationAPI == NULL || in_n1mKBars4EMA1 <= 0 || in_n1mKBars4EMA2 <= 0 || in_n1mKBars4DEA <= 0 || in_strContract.size() == 0)
    {
        return -100;
    }
    else
    {
        m_pMarketQuotation = in_pMarketQuotationAPI;
        m_n1mKBars = 0;
        m_n1mKBars4EMA1 = in_n1mKBars4EMA1;
        m_n1mKBars4EMA2 = in_n1mKBars4EMA2;
        m_n1mKBars4DEA = in_n1mKBars4DEA;
        m_strContract = in_strContract;
        m_arrayIndexValue.clear();
    }

    /*
    * 新线程计算指标值
    */
    /// 订阅行情
    APINamespace TThostFtdcInstrumentIDType t_chContract;
    memset(t_chContract, '\0', sizeof(t_chContract));
    if (m_strContract.size() >= sizeof(t_chContract))
    {
        return -300;
    }
    else
    {
        m_strContract.copy(t_chContract, m_strContract.size());
        if (in_pMarketQuotationAPI->subMarketDataSingle(t_chContract) < 0)
        {
            return -200;
        }
    }



    m_blAutoRun = true;
    std::thread autorun(&MarketQuotationIndex::caculate, this);
    autorun.detach();
    return 0;
}

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
void axapi::MQI_MACDIndex::caculate()
{
    char* t_strLogFuncName = "MQI_MACDIndex::caculate";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    //LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    std::string t_strContract = m_strContract;

#pragma region
    /*
    * 自定义指标计算部分
    */
    double t_EMAValue1, t_EMAValue2, t_DIFValue, tDEAValue;
    MQI_MACDIndexField t_objLatestIndexValue;
    t_objLatestIndexValue.BarSerials = NULL;
    t_objLatestIndexValue.indexValue = NULL;

    axapi::KMarketDataField *t_pMarketDataField = NULL;
    axapi::KMarketDataField *t_pMarketDataFieldLastest = NULL;

    while (m_blAutoRun)
    {
        sprintf_s(t_strLog, 500, "%s:MACD caculating%s...", t_strLogFuncName, t_strContract.c_str());
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
    sprintf_s(t_strLog, 500, "%s:MACD exit caculate%s", t_strLogFuncName, t_strContract.c_str());
    //LOG4CPLUS_TRACE(m_objLogger, t_strLog);
}

double axapi::MQI_MACDIndex::getIndexValue(int in_iCurrentOffset)
{
    char* t_strLogFuncName = "MQI_MACDIndex::getIndexValue";
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