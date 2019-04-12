#ifndef _TRADEAPI_HC_H_
#define _TRADEAPI_HC_H_
#pragma once

#ifdef TradeAPI_EXE
#define TradeAPI_EXPORT
#else
#ifdef TradeAPI_EXP
#define TradeAPI_EXPORT __declspec(dllexport)
#else
#define TradeAPI_EXPORT __declspec(dllimport)
#endif TradeAPI_EXP
#endif TradeAPI_EXE

#include <log4cplus/logger.h>
#include <log4cplus/configurator.h>
#include <log4cplus/helpers/loglog.h>
#include <log4cplus/helpers/stringhelper.h>
#include <log4cplus/helpers/sleep.h>
#include <log4cplus/loggingmacros.h>
#include <TradeAPI.h>
#include <Windows.h>
#include <vector>

#ifdef SQLITE3DATA
#include    "CppSQLite3/CppSQLite3.h"
#endif  SQLITE3DATA

#ifndef KSV6T_TRADEAPI_HC
#ifndef CTP_TRADEAPI_HC
#define     APINamespace
#else   CTP_TRADEAPI_HC
#include    <CTPTdMd_API_common/ThostFtdcTraderApi.h>
#define     APINamespace
#endif  CTP_TRADEAPI_HC
#else   KSV6T_TRADEAPI_HC
#include    <KSv6tTdMd_API_common/KSTradeAPI.h>
#define     APINamespace KingstarAPI::
#endif  KSV6T_TRADEAPI_HC

namespace axapi
{
    class TradeAPI_HC :
        public TradeAPI
    {
    public:
        TradeAPI_HC();
        ~TradeAPI_HC();
    };
}

#endif _TRADEAPI_HC_H_