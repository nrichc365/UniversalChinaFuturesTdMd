#ifndef _TradeAPITypeDefine_H_
#define _TradeAPITypeDefine_H_
#pragma once
namespace axapi
{
	//-----------------------------程序执行状态--------------------------
	//接口运行状态
	#define _TradeAPI_STATUS_Uninitial      '0'  //未初始化
	#define _TradeAPI_STATUS_Initiating     '1'  //初始化中
	#define _TradeAPI_STATUS_Initiated      '2'  //已初始化
	#define _TradeAPI_STATUS_UndefinedError '3'  //未定义错误
	#define _TradeAPI_STATUS_Terminate      '9'  //中断
	#define _TradeAPI_STATUS_Ready          'a'  //可下单
	#define _TradeAPI_STATUS_LinkError      'A'  //链接错误

	//------------------------------报单定义--------------------------
	//买卖标志
	#define ORDER_DIRECTION_BUY  0
	#define ORDER_DIRECTION_SELL 1
	//开平标志
	#define ORDER_OFFSETFLAG_OPEN   0
	#define ORDER_OFFSETFLAG_OFFSET 1
	#define ORDER_OFFSETFLAG_OFFSET_TODAY 3
	//报单类型
	//限价
	#define ORDER_LIMITPRICE 1
	//市价
	#define ORDER_ANYPRICE 2
	//对价
	#define ORDER_AGAINSTPRICE 3
	//无限极大值,用于一些调用后的错误返回
	#define Unlimite_Big 9999999999 
	//无限极小值,用于一些调用后的错误返回
	#define Unlimite_Small -99999 

	#define WAIT_RSP_SECONDS 5000

	//------------------------------日志定义--------------------------
    #define LOG_TRACE 0
    #define LOG_DEBUG 1
    #define LOG_INFO  2
    #define LOG_WARN  3
    #define LOG_ERROR 4
    #define LOG_FATAL 5
}
#endif _TradeAPITypeDefine_H_