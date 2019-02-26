#define MQI_RELATIVESTRENGTHINDEX_EXE
#include "MQI_RelativeStrengthIndex.h"
#include <iostream>
#include <thread>

axapi::MQI_RelativeStrengthIndex::MQI_RelativeStrengthIndex()
{
}

axapi::MQI_RelativeStrengthIndex::~MQI_RelativeStrengthIndex()
{
}

/// 外接行情数据初始化，调用后开始计算指标值
int axapi::MQI_RelativeStrengthIndex::initialize(axapi::MarketQuotationAPI *in_pMarketQuotationAPI, unsigned int in_n1mKBars, std::string in_strContract)
{
    if (m_blAutoRun == true)
    {
        m_blAutoRun = false;
        WaitForSingleObject(m_hCaculateRuning, INFINITE);
    }
    /// 传入参数错误则返回错误
    /*if (in_pMarketQuotationAPI == NULL || in_n1mKBars <= 0 || in_strContract.size() == 0)
    {
    return -100;
    }
    else*/
    {
        m_pMarketQuotation = in_pMarketQuotationAPI;
        m_n1mKBars = in_n1mKBars;
        m_strContract = in_strContract;
        m_arrayIndexValue.clear();
    }

    /*
    * 新线程计算指标值
    */
    /// 订阅行情
    APINamespace TThostFtdcInstrumentIDType t_chContract;
    m_strContract.copy(t_chContract, sizeof(t_chContract));
    /*if (in_pMarketQuotationAPI->subMarketDataSingle(t_chContract) <= 0)
    {
    return -200;
    }*/



    m_blAutoRun = true;
    std::thread autorun(&MQI_RelativeStrengthIndex::caculate, this);
    autorun.detach();
    return 0;
}

void axapi::MQI_RelativeStrengthIndex::caculate()
{
    std::string t_strContract = m_strContract;
    while (m_blAutoRun)
    {
        std::cout << "RSI caculating " << t_strContract.c_str() << "..." << std::endl;
        Sleep(1000);
    }
    SetEvent(m_hCaculateRuning);
    std::cout << "RSI exit caculate " << t_strContract.c_str() << std::endl;
}