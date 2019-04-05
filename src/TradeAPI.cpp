#define TradeAPI_EXP
#define CTP_TRADEAPI
//#define KSV6T_TRADEAPI
#define MEMORYDATA
//#define SQLITE3DATA
#define LOGGER_NAME "TradeAPI"
#include "TradeAPITypeDefine.h"
#include "TradeAPI.h"
#include <iostream>
#include <string>
#include <stdio.h>

/// 用此函数初始化,则状态不正常,必须使用另外一种重载过的初始化函数
axapi::TradeAPI::TradeAPI(void)
{
    char* t_strLogFuncName = "TradeAPI::TradeAPI";
    if (initialLog() == 0)
    {
        //LOG4CPLUS_FATAL(m_root, "initialize LOG OK");
        LOG4CPLUS_FATAL(m_objLogger, "initialize LOG OK");
    }
    else
    {
        LOG4CPLUS_FATAL(m_objLogger, "initialize LOG ERROR");
    }
#ifdef SQLITE3DATA
    if (initialDB() == 0)
    {
        //LOG4CPLUS_FATAL(m_root, "initialize DB OK");
        LOG4CPLUS_FATAL(m_objLogger, "initialize DB OK");
    }
#endif SQLITE3DATA
    setStatus(_TradeAPI_STATUS_Uninitial);
}

/// 初始化日志,数据库,建立连接
axapi::TradeAPI::TradeAPI(APINamespace TThostFtdcBrokerIDType in_chBrokerID, APINamespace TThostFtdcUserIDType in_chUserID, APINamespace TThostFtdcPasswordType in_chPassword, char* in_pFrontAddr)
{
    char* t_strLogFuncName = "TradeAPI::TradeAPI";
    if (initialLog() == 0)
    {
        //LOG4CPLUS_FATAL(m_root, "initialize LOG OK");
        LOG4CPLUS_FATAL(m_objLogger, "initialize LOG OK");
    }
    else
    {
        LOG4CPLUS_FATAL(m_objLogger, "initialize LOG ERROR");
    }
#ifdef SQLITE3DATA
    if (initialDB() == 0)
    {
        //LOG4CPLUS_FATAL(m_root, "initialize DB OK");
        LOG4CPLUS_FATAL(m_objLogger, "initialize DB OK");
    }
#endif SQLITE3DATA
    /// 设置登陆信息
    setUserInfo(in_chBrokerID, in_chUserID, in_chPassword);
    /// 登陆下单服务器
    initiate(in_pFrontAddr);
    /// 查询报单,成交等信息
    int t_nQryCustDone = queryCustDone();
    while (!checkCompletedQueryRequestID(t_nQryCustDone))
    {
        log4cplus::helpers::sleep(1);
    }
    int t_nQryCustOrder = queryCustOrder();
    while (!checkCompletedQueryRequestID(t_nQryCustOrder))
    {
        log4cplus::helpers::sleep(1);
    }
    int t_nQryCustFund = queryCustFund();
    while (!checkCompletedQueryRequestID(t_nQryCustFund))
    {
        log4cplus::helpers::sleep(1);
    }
    int t_nQryInstrumentInfo = queryInstrument();
    while (!checkCompletedQueryRequestID(t_nQryInstrumentInfo))
    {
        log4cplus::helpers::sleep(1);
    }
    //queryCustSTKHoldDetail();
}

/// 断开服务器,断开数据库
axapi::TradeAPI::~TradeAPI(void)
{
    char* t_strLogFuncName = "TradeAPI::~TradeAPI";
    LOG4CPLUS_WARN(m_objLogger, "destructor...");
    setStatus(_TradeAPI_STATUS_Uninitial);

    /// 释放内存
#ifdef MEMORYDATA
    m_vOrderList.clear();
    m_vTradeList.clear();
    m_vInvestorPositionList.clear();
#endif MEMORYDATA

    /// 下单服务器登出
    APINamespace CThostFtdcUserLogoutField t_UserLogout;
    memset(&t_UserLogout.BrokerID, 0, sizeof(t_UserLogout));
    strcpy_s(t_UserLogout.UserID, sizeof(t_UserLogout.UserID), m_strUserID);
    strcpy_s(t_UserLogout.BrokerID, sizeof(t_UserLogout.BrokerID), m_strBrokerID);
    m_pUserApi->ReqUserLogout(&t_UserLogout, m_nRequestID++);
}


/// 返回状态,用于判断下单部分状态
char axapi::TradeAPI::getStatus()
{
    return m_chStatus;
}

/// 状态操作,用于初始化时
int axapi::TradeAPI::setStatus(char in_chStatus)
{
    if (in_chStatus == _TradeAPI_STATUS_Uninitial ||
        in_chStatus == _TradeAPI_STATUS_Initiating ||
        in_chStatus == _TradeAPI_STATUS_Initiated ||
        in_chStatus == _TradeAPI_STATUS_UndefinedError ||
        in_chStatus == _TradeAPI_STATUS_Terminate ||
        in_chStatus == _TradeAPI_STATUS_Ready ||
        in_chStatus == _TradeAPI_STATUS_LinkError)
    {
        m_chStatus = in_chStatus;
        return 0;
    }
    return -100;
}

/// 启动开始设置日志相关配置
int axapi::TradeAPI::initialLog()
{
    char* t_strLogFuncName = "TradeAPI::initialLog";
    log4cplus::initialize();
    log4cplus::helpers::LogLog::getLogLog()->setInternalDebugging(true);
    m_root = log4cplus::Logger::getRoot();
    m_objLogger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT(LOGGER_NAME));
    try {
        log4cplus::ConfigureAndWatchThread configureThread(
            LOG4CPLUS_TEXT("log4cplus.properties1"), 5 * 1000);
    }
    catch (std::exception e) {
        LOG4CPLUS_FATAL(m_root, "initialLog exception");
        return -100;
    }
    return 0;
}

/// 启动开始连接数据库等操作
#ifdef SQLITE3DATA
int axapi::TradeAPI::initialDB()
{
    char* t_strLogFuncName = "TradeAPI::initialDB";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s:initializing DB...", t_strLogFuncName);
    LOG4CPLUS_WARN(m_objLogger, t_strLog);

    /// 定义数据库文件名,不可更改
    const char* t_dbfile = ".//trade.db";
    /*if(sqlite3_open("trade.db", &m_pDB))
    {
    sprintf_s(t_strLog, 500, "Can't open database: %s", sqlite3_errmsg(m_pDB));
    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
    return -1;
    }*/

    /*
    * 打开库,检查库结构是否存在,并清理数据表
    *   不存在则重新建立;
    */
    try
    {
        sprintf_s(t_strLog, 500, "%s:sqlite3 version:%s", t_strLogFuncName, m_objDB.SQLiteVersion());
        LOG4CPLUS_INFO(m_objLogger, t_strLog);

        /// 判断数据库文件有效性,不存在则初始化数据结构
        m_objDB.open(t_dbfile);
        if (!m_objDB.tableExists("cust_fund"))
        {
            if (createDB() != 0)
            {
                sprintf_s(t_strLog, 500, "%s:badly create tables", t_strLogFuncName);
                LOG4CPLUS_FATAL(m_objLogger, t_strLog);
            }
            else
            {
                sprintf_s(t_strLog, 500, "%s:successfully create tables", t_strLogFuncName);
                LOG4CPLUS_INFO(m_objLogger, t_strLog);
            }
        }

        /// 清理数据结构
        //clearDB();
    }
    catch (CppSQLite3Exception& e)
    {
        sprintf_s(t_strLog, 500, "%s:%d:%s", t_strLogFuncName, e.errorCode(), e.errorMessage());
        LOG4CPLUS_FATAL(m_objLogger, t_strLog);
        return -100;
    }
    return 0;
}
#endif SQLITE3DATA

/// 初始化时调用清理历史记录,ret:0 正常
#ifdef SQLITE3DATA
int axapi::TradeAPI::clearDB()
{
    char* t_strLogFuncName = "TradeAPI::clearDB";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s:clearing tables...", t_strLogFuncName);
    LOG4CPLUS_WARN(m_objLogger, t_strLog);

    char* tables[20];
    /// 初始化执行语句
    for (int i = 0; i < 20; i++)
    {
        tables[i] = NULL;
    }
    /// 定义要清理的表
    tables[0] = "cust_done";
    tables[1] = "cust_fund";
    tables[2] = "cust_order";
    tables[3] = "cust_stock_hold_detail";
    tables[4] = "cust_stock_hold";
    tables[5] = "market_data";
    tables[6] = "instrument";

    /// 清理所有表
    for (int i = 0; i < 20; i++)
    {
        if (tables[i] != NULL)
        {
            if (clearDB(tables[i]) == -1)
            {
                return -1;
            }
        }
    }
    return 0;
}
#endif SQLITE3DATA

/// 初始化时调用清理历史记录,ret:0 正常
#ifdef SQLITE3DATA
int axapi::TradeAPI::clearDB(char* in_strTablename)
{
    char* t_strLogFuncName = "TradeAPI::clearDB";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s:clearing tables...", t_strLogFuncName);
    LOG4CPLUS_WARN(m_objLogger, t_strLog);

    char* sql[20];
    /// 初始化执行语句
    for (int i = 0; i < 20; i++)
    {
        sql[i] = NULL;
    }
    /// 定义要清理的表
    sql[0] = "delete from cust_done";
    sql[1] = "delete from cust_fund";
    sql[2] = "delete from cust_order";
    sql[3] = "delete from cust_stock_hold_detail";
    sql[4] = "delete from cust_stock_hold";
    sql[5] = "delete from market_data";
    sql[6] = "delete from instrument";

    /// 执行所有清理表语句
    try
    {
        int i = 19;
        if (strcmp(in_strTablename, "cust_done") == 0)
        {
            i = 0;
        }
        else if (strcmp(in_strTablename, "cust_fund") == 0)
        {
            i = 1;
        }
        else if (strcmp(in_strTablename, "cust_order") == 0)
        {
            i = 2;
        }
        else if (strcmp(in_strTablename, "cust_stock_hold_detail") == 0)
        {
            i = 3;
        }
        else if (strcmp(in_strTablename, "cust_stock_hold") == 0)
        {
            i = 4;
        }
        else if (strcmp(in_strTablename, "market_data") == 0)
        {
            i = 5;
        }
        else if (strcmp(in_strTablename, "instrument") == 0)
        {
            i = 6;
        }
        if (sql[i] != NULL)
        {
            int nRows = m_objDB.execDML(sql[i]);
            sprintf_s(t_strLog, 500, "%s:%s-%d rows deleted", t_strLogFuncName, sql[i], nRows);
            LOG4CPLUS_INFO(m_objLogger, t_strLog);
        }
    }
    catch (CppSQLite3Exception& e)
    {
        sprintf_s(t_strLog, 500, "%s:%d-%s", t_strLogFuncName, e.errorCode(), e.errorMessage());
        LOG4CPLUS_FATAL(m_objLogger, t_strLog);
        return -1;
    }
    return 0;
}
#endif SQLITE3DATA

/// 数据文件不存在创建数据相关结构,ret:0 正常
#ifdef SQLITE3DATA
int axapi::TradeAPI::createDB()
{
    char* t_strLogFuncName = "TradeAPI::createDB";
    char t_strLog[2000];
    sprintf_s(t_strLog, 500, "%s:createing tables...", t_strLogFuncName);
    LOG4CPLUS_WARN(m_objLogger, "createing tables...");

    char* sql[20];
    /// 初始化执行语句
    for (int i = 0; i < 20; i++)
    {
        sql[i] = NULL;
    }
    /// 定义要建立的表
    sql[0] = "create table cust_done(BrokerID char(11), InvestorID char(13), InstrumentID char(31), OrderRef char(13), UserID char(16), ExchangeID char(9), TradeID char(21), Direction char(1), OrderSysID char(21), ParticipantID char(11), ClientID char(11), TradingRole char(1), ExchangeInstID char(31), OffsetFlag char(1), HedgeFlag char(1), Price double, Volume int, TradeDate char(9), TradeTime char(9), TradeType char(1), PriceSource char(1), TraderID char(21), OrderLocalID char(13), ClearingPartID char(11), BusinessUnit char(21), SequenceNo int, TradingDay char(9), SettlementID int, BrokerOrderSeq int, TradeSource char(1));";
    sql[1] = "create table cust_fund(BrokerID char(11),AccountID char(13),PreMortgage double,PreCredit double,PreDeposit double,PreBalance double,PreMargin double,InterestBase double,Interest double,Deposit double,Withdraw double,FrozenMargin double,FrozenCash double,FrozenCommission double,CurrMargin double,CashIn double,Commission double,CloseProfit double,PositionProfit double,Balance double,Available double,WithdrawQuota double,Reserve double,TradingDay char(9),SettlementID int,Credit double,Mortgage double,ExchangeMargin double,DeliveryMargin double,ExchangeDeliveryMargin double,ReserveBalance double)";
    sql[2] = "create table cust_order(BrokerID char(11), InvestorID char(13), InstrumentID char(31), OrderRef char(13), UserID char(16), OrderPriceType char(1), Direction char(1), CombOffsetFlag char(5), CombHedgeFlag char(5), LimitPrice double, VolumeTotalOriginal int, TimeCondition char(1), GTDDate char(9), VolumeCondition char(1), MinVolume int, ContingentCondition char(1), StopPrice double, ForceCloseReason char(1), IsAutoSuspend int, BusinessUnit char(21), RequestID int, OrderLocalID char(13), ExchangeID char(9), ParticipantID char(11), ClientID char(11), ExchangeInstID char(31), TraderID char(21), InstallID int, OrderSubmitStatus char(1), NotifySequence int, TradingDay char(9), SettlementID int, OrderSysID char(21), OrderSource char(1), OrderStatus char(1), OrderType char(1), VolumeTraded int, VolumeTotal int, InsertDate char(9), InsertTime char(9), ActiveTime char(9), SuspendTime char(9), UpdateTime char(9), CancelTime char(9), ActiveTraderID char(21), ClearingPartID char(11), SequenceNo int, FrontID int, SessionID int, UserProductInfo char(11), StatusMsg char(81), UserForceClose int, ActiveUserID char(16), BrokerOrderSeq int, RelativeOrderSysID char(21), ZCETotalTradedVolume int, IsSwapOrder int);";
    sql[3] = "create table cust_stock_hold_detail( InstrumentID char(31), BrokerID char(11), InvestorID char(13), HedgeFlag char, Direction char, OpenDate char(9), TradeID char(21), Volume int, OpenPrice double, TradingDay char(9), SettlementID int, TradeType char, CombInstrumentID char(31), ExchangeID char(9), CloseProfitByDate double, CloseProfitByTrade double, PositionProfitByDate double, PositionProfitByTrade double, Margin double, ExchMargin double, MarginRateByMoney double, MarginRateByVolume double, LastSettlementPrice double, SettlementPrice double, CloseVolume int, CloseAmount double);";
    sql[4] = "create table cust_stock_hold( InstrumentID char(31), BrokerID char(11), InvestorID char(13), PosiDirection char, HedgeFlag char, PositionDate char, YdPosition int, Position int, LongFrozen int, ShortFrozen int, LongFrozenAmount double, ShortFrozenAmount double, OpenVolume int, CloseVolume int, OpenAmount double, CloseAmount double, PositionCost double, PreMargin double, UseMargin double, FrozenMargin double, FrozenCash double, FrozenCommission double, CashIn double, Commission double, CloseProfit double, PositionProfit double, PreSettlementPrice double, SettlementPrice double, TradingDay char(9), SettlementID int, OpenCost double, ExchangeMargin double, CombPosition int, CombLongFrozen int, CombShortFrozen int, CloseProfitByDate double, CloseProfitByTrade double, TodayPosition int, MarginRateByMoney double, MarginRateByVolume double);";
    sql[5] = "create table market_data(TradingDay char(9), InstrumentID char(31), ExchangeID char(9), ExchangeInstID char(31), LastPrice double, PreSettlementPrice double, PreClosePrice double, PreOpenInterest double, OpenPrice double, HighestPrice double, LowestPrice double, Volume int, Turnover double, OpenInterest double, ClosePrice double, SettlementPrice double, UpperLimitPrice double, LowerLimitPrice double, PreDelta double, CurrDelta double, UpdateTime char(9), UpdateMillisec int, BidPrice1 double, BidVolume1 int, AskPrice1 double, AskVolume1 int, BidPrice2 double, BidVolume2 int, AskPrice2 double, AskVolume2 int, BidPrice3 double, BidVolume3 int, AskPrice3 double, AskVolume3 int, BidPrice4 double, BidVolume4 int, AskPrice4 double, AskVolume4 int, BidPrice5 double, BidVolume5 int, AskPrice5 double, AskVolume5 int, AveragePrice double, ActionDay char(9));";
    sql[6] = "create table instrument(InstrumentID char(31), ExchangeID char(9), InstrumentName char(21), ExchangeInstID char(31), ProductID char(31), ProductClass char, DeliveryYear int, DeliveryMonth int, MaxMarketOrderVolume int, MinMarketOrderVolume int, MaxLimitOrderVolume int, MinLimitOrderVolume int, VolumeMultiple int, PriceTick double, CreateDate char(9), OpenDate char(9), ExpireDate char(9), StartDelivDate char(9), EndDelivDate char(9), InstLifePhase char, IsTrading int, PositionType char, PositionDateType char, LongMarginRatio double, ShortMarginRatio double, MaxMarginSideAlgorithm char);";
    sql[7] = "create table InstrumentOffsetType(InstrumentID char(31), ExchangeID char(9) , OffsetTodayFlag char(1) , OffsetFlag char(1));";
    sql[8] = "insert into InstrumentOffsetType(ExchangeID, OffsetTodayFlag, OffsetFlag) values('SHFE', '0', '3');";
    sql[9] = "insert into InstrumentOffsetType(ExchangeID, OffsetTodayFlag, OffsetFlag) values('DCE', '0', '1');";
    sql[10] = "insert into InstrumentOffsetType(ExchangeID, OffsetTodayFlag, OffsetFlag) values('CZCE', '0', '1');";
    sql[11] = "insert into InstrumentOffsetType(ExchangeID, OffsetTodayFlag, OffsetFlag) values('CFFEX', '0', '1');";

    /// 执行所有新建表语句
    try
    {
        for (int i = 0; i < 20; i++)
        {
            if (sql[i] != NULL)
            {
                m_objDB.execDML(sql[i]);
                sprintf_s(t_strLog, 2000, "%s", sql[i]);
                LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
            }
        }
    }
    catch (CppSQLite3Exception& e)
    {
        sprintf_s(t_strLog, 2000, "%d:%s", e.errorCode(), e.errorMessage());
        LOG4CPLUS_FATAL(m_objLogger, t_strLog);
        return -1;
    }
    return 0;
}
#endif SQLITE3DATA

/// 用于向外部传递查询结果 p_SQLiteQueryResult为查询结果 in_pSQLStatement为查询语句
#ifdef SQLITE3DATA
void axapi::TradeAPI::queryDB(CppSQLite3Query* p_SQLiteQueryResult, char* in_strSQLStatement)
{
    char* t_strLogFuncName = "TradeAPI::queryDB(CppSQLite3Query*,char*)";
    char t_strLog[2000];
    try {
        std::stringstream t_strQrySQL;
        t_strQrySQL << in_strSQLStatement;
        std::string t_tmpstr = t_strQrySQL.str();
        const char* t_QrySQL = t_tmpstr.c_str();
        sprintf_s(t_strLog, 500, "%s:%s", t_strLogFuncName, t_QrySQL);
        LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
        *p_SQLiteQueryResult = m_objDB.execQuery(t_QrySQL);
    }
    catch (CppSQLite3Exception& e)
    {
        sprintf_s(t_strLog, 500, "%s:CppSQLite3错误|%d|%s", t_strLogFuncName, e.errorCode(), e.errorMessage());
        LOG4CPLUS_FATAL(m_objLogger, t_strLog);
    }
}
#endif SQLITE3DATA

#ifdef TEST
void TradeAPI::test()
{
    //char* errMsg;
    //int res = sqlite3_exec(m_pDB,"begin transaction;",0,0, &errMsg);
    //for (int i= 1; i < 10; ++i)
    //{
    //	std::stringstream strsql;
    //	strsql << "insert into test values(";
    //	strsql  << i << ");";
    //	std::string str = strsql.str();
    //	res = sqlite3_exec(m_pDB,str.c_str(),0,0, &errMsg);
    //	if (res != SQLITE_OK)
    //	{
    //		std::cout << "执行SQL 出错." << errMsg << std::endl;
    //	}
    //}
    //res = sqlite3_exec(m_pDB,"commit transaction;",0,0, &errMsg);
    //std::cout << "SQL成功执行."<< std::endl;
    SetCompletedQueryRequestID(1);
    SetCompletedQueryRequestID(10);
    SetCompletedQueryRequestID(11);
    SetCompletedQueryRequestID(33);
    SetCompletedQueryRequestID(14);
    SetCompletedQueryRequestID(21);
    SetCompletedQueryRequestID(51);

    if (CheckCompletedQueryRequestID(2))
    {
        LOG4CPLUS_DEBUG(m_objLogger, "true2");
    }
    if (CheckCompletedQueryRequestID(10))
    {
        LOG4CPLUS_DEBUG(m_objLogger, "true10");
    }
    if (CheckCompletedQueryRequestID(51))
    {
        LOG4CPLUS_DEBUG(m_objLogger, "true51");
    }
    if (CheckCompletedQueryRequestID(52))
    {
        LOG4CPLUS_DEBUG(m_objLogger, "true52");
    }
}
#endif TEST

#ifdef TEST
void TradeAPI::test2()
{
    char t_strLog[500];
    try
    {
        int nRows = m_objDB.execDML("insert into test values (200);");
        sprintf_s(t_strLog, 500, "%d rows inserted", nRows);
        LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
    }
    catch (CppSQLite3Exception& e)
    {
        sprintf_s(t_strLog, 500, "%d:%s", e.errorCode(), e.errorMessage());
        LOG4CPLUS_FATAL(m_objLogger, t_strLog);
    }
}
#endif TEST

#ifdef TEST
void TradeAPI::test3()
{
    char t_strLog[500];
    std::stringstream t_strQrySQL;
    t_strQrySQL << "select * from test where key > ";
    t_strQrySQL << 3 << ";";
    string t_tmpstr = t_strQrySQL.str();
    const char* t_QrySQL = t_tmpstr.c_str();
    sprintf_s(t_strLog, 500, "TradeAPI::test3:%s", t_QrySQL);
    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
    LOG4CPLUS_DEBUG(m_objLogger, t_QrySQL);
    //CppSQLite3Query q = m_objDB.execQuery("select * from test;");
    CppSQLite3Query q = m_objDB.execQuery(t_QrySQL);

    for (int i = 0; i < q.numFields(); i++)
    {
        cout << q.fieldName(i) << "(" << q.fieldDataType(i) << ")|";
    }
    cout << endl;

    while (!q.eof())
    {
        for (int i = 0; i < q.numFields(); i++)
        {
            cout << q.fieldValue(i) << "|";
        }
        cout << "end" << endl;
        q.nextRow();
    }
}
#endif TEST

/// 查看所提供查询ID是否完成
bool axapi::TradeAPI::checkCompletedQueryRequestID(int in_nQuery_RequestID)
{
    char* t_strLogFuncName = "TradeAPI::checkCompletedQueryRequestID";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s:%d", t_strLogFuncName, in_nQuery_RequestID);
    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);

    //查找
    std::vector<int>::iterator t_iterator = find(m_nCompleted_Query_RequestID.begin(), m_nCompleted_Query_RequestID.end(), in_nQuery_RequestID);
    return t_iterator != m_nCompleted_Query_RequestID.end();
}

/// 将完成查询请求ID加入完成ID列表
void axapi::TradeAPI::setCompletedQueryRequestID(int in_nCompleted_Query_RequestID)
{
    char* t_strLogFuncName = "TradeAPI::setCompletedQueryRequestID";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s:%d", t_strLogFuncName, in_nCompleted_Query_RequestID);
    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);

    m_nCompleted_Query_RequestID.push_back(in_nCompleted_Query_RequestID);
    /*vector<int>::iterator it;
    for(it=m_nCompleted_Query_RequestID.begin(); it!=m_nCompleted_Query_RequestID.end(); it++)
    {
    cout << *it << endl;
    }*/
}

/// 设置用户信息
int axapi::TradeAPI::setUserInfo(APINamespace TThostFtdcBrokerIDType in_strBrokerID, APINamespace TThostFtdcUserIDType in_strUserID, APINamespace TThostFtdcPasswordType in_strPassword)
{
    strcpy_s(m_strBrokerID, sizeof(m_strBrokerID), in_strBrokerID);
    strcpy_s(m_strUserID, sizeof(m_strUserID), in_strUserID);
    strcpy_s(m_strPassword, sizeof(m_strPassword), in_strPassword);
    return 0;
}

/// 链接服务器
int axapi::TradeAPI::initiate(char* in_strFrontAddr)
{
    char* t_strLogFuncName = "TradeAPI::initiate";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);
    m_nRequestID = 100;

    /// 生成登陆信息
    APINamespace CThostFtdcReqUserLoginField ReqUserLoginField;
    memset(&ReqUserLoginField, 0, sizeof(ReqUserLoginField));
    strcpy_s(ReqUserLoginField.BrokerID, sizeof(ReqUserLoginField.BrokerID), m_strBrokerID);
    strcpy_s(ReqUserLoginField.UserID, sizeof(ReqUserLoginField.UserID), m_strUserID);
    strcpy_s(ReqUserLoginField.Password, sizeof(ReqUserLoginField.Password), m_strPassword);

    /// 订阅
    m_pUserApi = APINamespace CThostFtdcTraderApi::CreateFtdcTraderApi("TradeLog\\");
    m_hInitEvent = CreateEvent(NULL, true, false, NULL);
    m_pUserApi->RegisterSpi(this);
    m_pUserApi->SubscribePublicTopic(APINamespace THOST_TERT_QUICK);
    m_pUserApi->SubscribePrivateTopic(APINamespace THOST_TERT_QUICK);

    /// 登陆
    sprintf_s(t_strLog, 500, "%s:Connecting...", t_strLogFuncName);
    LOG4CPLUS_INFO(m_objLogger, t_strLog);
    m_pUserApi->RegisterFront(in_strFrontAddr);
    m_pUserApi->Init();
    WaitForSingleObject(m_hInitEvent, INFINITE);

#ifdef CTP_TRADEAPI
    /// 结算单确认
    APINamespace CThostFtdcQrySettlementInfoField ReqSettlementInfoField;
    memset(&ReqSettlementInfoField, 0, sizeof(ReqSettlementInfoField));
    strcpy_s(ReqSettlementInfoField.BrokerID, sizeof(ReqSettlementInfoField.BrokerID), m_strBrokerID);
    strcpy_s(ReqSettlementInfoField.InvestorID, sizeof(ReqSettlementInfoField.InvestorID), m_strUserID);
    strcpy_s(ReqSettlementInfoField.TradingDay, sizeof(ReqSettlementInfoField.TradingDay), m_strTradingDay);

    APINamespace CThostFtdcQrySettlementInfoConfirmField ReqQrySettlementInfoConfirmField;
    memset(&ReqQrySettlementInfoConfirmField, 0, sizeof(ReqQrySettlementInfoConfirmField));
    strcpy_s(ReqQrySettlementInfoConfirmField.BrokerID, sizeof(ReqQrySettlementInfoConfirmField.BrokerID), m_strBrokerID);
    strcpy_s(ReqQrySettlementInfoConfirmField.InvestorID, sizeof(ReqQrySettlementInfoConfirmField.InvestorID), m_strUserID);

    APINamespace CThostFtdcSettlementInfoConfirmField ReqSettlementInfoConfirmField;
    memset(&ReqSettlementInfoConfirmField, 0, sizeof(ReqSettlementInfoConfirmField));
    strcpy_s(ReqSettlementInfoConfirmField.BrokerID, sizeof(ReqSettlementInfoConfirmField.BrokerID), m_strBrokerID);
    strcpy_s(ReqSettlementInfoConfirmField.InvestorID, sizeof(ReqSettlementInfoConfirmField.InvestorID), m_strUserID);
    strcpy_s(ReqSettlementInfoConfirmField.ConfirmDate, sizeof(ReqSettlementInfoConfirmField.ConfirmDate), m_strTradingDay);
    strcpy_s(ReqSettlementInfoConfirmField.ConfirmTime, sizeof(ReqSettlementInfoConfirmField.ConfirmTime), "11:11:11");

    sprintf_s(t_strLog, 500, "%s:%s|%s|%s|Require Settlement Info...", t_strLogFuncName, ReqSettlementInfoField.BrokerID, ReqSettlementInfoField.InvestorID, ReqSettlementInfoField.TradingDay);
    LOG4CPLUS_INFO(m_objLogger, t_strLog);
    int t_nQrySettlementInfo = m_nRequestID++;
    m_pUserApi->ReqQrySettlementInfo(&ReqSettlementInfoField, t_nQrySettlementInfo);
    while (!checkCompletedQueryRequestID(t_nQrySettlementInfo))
    {
        log4cplus::helpers::sleep(1);
    }

    sprintf_s(t_strLog, 500, "%s:%s|%s|SettlementInfoConfirming...", t_strLogFuncName, ReqSettlementInfoConfirmField.BrokerID, ReqSettlementInfoConfirmField.InvestorID);
    LOG4CPLUS_INFO(m_objLogger, t_strLog);
    int t_nQrySettlementInfoConfirm = m_nRequestID++;
    m_pUserApi->ReqSettlementInfoConfirm(&ReqSettlementInfoConfirmField, t_nQrySettlementInfoConfirm);
    while (!checkCompletedQueryRequestID(t_nQrySettlementInfoConfirm))
    {
        log4cplus::helpers::sleep(1);
    }

#endif CTP_TRADEAPI

    // 确定登陆是否成功决定是否是否能下单
    if (getStatus() == _TradeAPI_STATUS_LinkError)
    {
        sprintf_s(t_strLog, 500, "%s:Connecting is error", t_strLogFuncName);
        LOG4CPLUS_FATAL(m_objLogger, t_strLog);
        return -100;
    }
    else if (getStatus() == _TradeAPI_STATUS_Initiated)
    {
        sprintf_s(t_strLog, 500, "%s:Connect has been created", t_strLogFuncName);
        LOG4CPLUS_WARN(m_objLogger, t_strLog);
        return 0;
    }
    return -100;
}

/// 设置下单结构
void axapi::TradeAPI::setOrderInfo(char* in_strContract, int in_nDirection, int in_nOffsetFlag, int in_nOrderTypeFlag, int in_nOrderAmount, double in_dOrderPrice)
{
    char* t_strLogFuncName = "TradeAPI::setOrderInfo";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s:%s|%d|%d|%d|%d|%f", t_strLogFuncName, in_strContract, in_nDirection, in_nOffsetFlag, in_nOrderTypeFlag, in_nOrderAmount, in_dOrderPrice);
    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);

    /// 报单信息,填写必要信息,未填写refinfo
    memset(&m_objOrder, 0, sizeof(m_objOrder));
    strcpy_s(m_objOrder.BrokerID, sizeof(m_objOrder.BrokerID), m_strBrokerID);
    strcpy_s(m_objOrder.InvestorID, sizeof(m_objOrder.InvestorID), m_strUserID);
    strcpy_s(m_objOrder.InstrumentID, sizeof(m_objOrder.InstrumentID), in_strContract);
    strcpy_s(m_objOrder.UserID, sizeof(m_objOrder.UserID), m_strUserID);
    switch (in_nOrderTypeFlag)
    {
    case ORDER_LIMITPRICE:
        m_objOrder.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
        break;
    case ORDER_ANYPRICE:
        m_objOrder.OrderPriceType = THOST_FTDC_OPT_AnyPrice;
        break;
    case ORDER_AGAINSTPRICE:
        m_objOrder.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
        break;
    }
    switch (in_nDirection)
    {
    case ORDER_DIRECTION_SELL:
        m_objOrder.Direction = THOST_FTDC_D_Sell;
        break;
    case ORDER_DIRECTION_BUY:
        m_objOrder.Direction = THOST_FTDC_D_Buy;
        break;
    }
    switch (in_nOffsetFlag)
    {
    case ORDER_OFFSETFLAG_OPEN:
        m_objOrder.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
        break;
    case ORDER_OFFSETFLAG_OFFSET:
        m_objOrder.CombOffsetFlag[0] = THOST_FTDC_OF_Close;
        break;
    case ORDER_OFFSETFLAG_OFFSET_TODAY:
        m_objOrder.CombOffsetFlag[0] = THOST_FTDC_OF_CloseToday;
    }
    m_objOrder.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
    m_objOrder.VolumeTotalOriginal = in_nOrderAmount;
    m_objOrder.TimeCondition = THOST_FTDC_TC_GFD;
    strcpy_s(m_objOrder.GTDDate, sizeof(m_objOrder.GTDDate), "");
    m_objOrder.VolumeCondition = THOST_FTDC_VC_AV;
    m_objOrder.MinVolume = 0;
    m_objOrder.ContingentCondition = THOST_FTDC_CC_Immediately;
    m_objOrder.StopPrice = 0;
    m_objOrder.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
    m_objOrder.IsAutoSuspend = 0;
    m_objOrder.LimitPrice = in_dOrderPrice;

    if (getStatus() == _TradeAPI_STATUS_Initiated)
    {
        setStatus(_TradeAPI_STATUS_Ready);
    }
    /// 撤单信息
    //memset(&m_objReverseOrder, 0, sizeof(m_objReverseOrder));
    //strcpy_s(m_objReverseOrder.BrokerID,     sizeof(m_objReverseOrder.BrokerID),     m_chBrokerID);
    //strcpy_s(m_objReverseOrder.InvestorID,   sizeof(m_objReverseOrder.InvestorID),   m_chUserID);
    //strcpy_s(m_objReverseOrder.InstrumentID, sizeof(m_objReverseOrder.InstrumentID), in_chContract);
    //strcpy_s(m_objReverseOrder.UserID,       sizeof(m_objReverseOrder.UserID),       m_chUserID);
    //m_objReverseOrder.ActionFlag = THOST_FTDC_AF_Delete;

    /// 查询信息
    //memset(&m_objQryOrder, 0, sizeof(m_objQryOrder));
    //strcpy_s(m_objQryOrder.BrokerID,     sizeof(m_objQryOrder.BrokerID),     m_chBrokerID);
    //strcpy_s(m_objQryOrder.InvestorID,   sizeof(m_objQryOrder.InvestorID),   m_chUserID);
    //strcpy_s(m_objQryOrder.InstrumentID, sizeof(m_objQryOrder.InstrumentID), in_chContract);
}

/// 获得合约当前价,用于下单函数中取价格
double axapi::TradeAPI::getMarketData(char* in_strContract)
{
    char* t_strLogFuncName = "TradeAPI::getMarketData";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    /// 合约行情结构设置
    APINamespace CThostFtdcQryDepthMarketDataField t_MarketDataField;
    memset(&t_MarketDataField, 0, sizeof(t_MarketDataField));
    strcpy_s(t_MarketDataField.InstrumentID, sizeof(t_MarketDataField.InstrumentID), in_strContract);

    /// 查询行情
    ResetEvent(m_hInitEvent);
    m_pUserApi->ReqQryDepthMarketData(&t_MarketDataField, m_nRequestID++);
    WaitForSingleObject(m_hInitEvent, 10000);

    if (strcmp(m_LatestPrice.InstrumentID, in_strContract) == 0)
    {
        return m_LatestPrice.LastPrice;
    }
    return -100;
}

/// 下单 返回Requestid,参数依次为in_chContract合约、in_nDirection方向、in_nOffsetFlag开平标志、in_nOrderTypeFlag报单类型、in_nOrderAmount报单数量、in_dOrderPrice报单价格
int axapi::TradeAPI::MyOrdering(char* in_strContract, int in_nDirection, int in_nOffsetFlag, int in_nOrderTypeFlag, int in_nOrderAmount, double in_dOrderPrice, long *in_plOrderRef)
{
    char* t_strLogFuncName = "TradeAPI::MyOrdering";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    /// 下单结构设置
    setOrderInfo(in_strContract, in_nDirection, in_nOffsetFlag, in_nOrderTypeFlag, in_nOrderAmount, in_dOrderPrice);
    if (getStatus() != _TradeAPI_STATUS_Ready)
    {
        sprintf_s(t_strLog, 500, "%s:Can't Ordering, Status is %c", t_strLogFuncName, getStatus());
        LOG4CPLUS_FATAL(m_objLogger, t_strLog);
        return -1;
    }

    // 20180813 niuchao 加速不取行情
    /*if(getMarketData(m_objOrder.InstrumentID) < 0)
    {
    sprintf_s(t_strLog, 500, "%s:Can't Ordering, GetMarketData Error %c", t_strLogFuncName, -100);
    LOG4CPLUS_INFO(m_objLogger, t_strLog);
    return -1;
    }*/

    /// 根据行情设置报单价位
    if (in_nOrderTypeFlag == ORDER_ANYPRICE && m_objOrder.OrderPriceType == THOST_FTDC_OPT_AnyPrice)
    {
        if (m_objOrder.Direction == THOST_FTDC_D_Buy)
        {
            sprintf_s(t_strLog, 500, "%s:买单下单 市价单", t_strLogFuncName);
            LOG4CPLUS_INFO(m_objLogger, t_strLog);
            m_objOrder.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
            // 20180813 niuchao 加速不取行情 m_objOrder.LimitPrice = m_LatestPrice.UpperLimitPrice;
        }
        else if (m_objOrder.Direction == THOST_FTDC_D_Sell)
        {
            sprintf_s(t_strLog, 500, "%s:卖单下单 市价单", t_strLogFuncName);
            LOG4CPLUS_INFO(m_objLogger, t_strLog);
            m_objOrder.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
            // 20180813 niuchao 加速不取行情 m_objOrder.LimitPrice = m_LatestPrice.LowerLimitPrice;
        }
    }
    else if (in_nOrderTypeFlag == ORDER_LIMITPRICE && m_objOrder.OrderPriceType == THOST_FTDC_OPT_LimitPrice)
    {
        if (m_objOrder.Direction == THOST_FTDC_D_Buy)
        {
            sprintf_s(t_strLog, 500, "%s:买单下单 限价单", t_strLogFuncName);
            LOG4CPLUS_INFO(m_objLogger, t_strLog);
            m_objOrder.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
        }
        else if (m_objOrder.Direction == THOST_FTDC_D_Sell)
        {
            sprintf_s(t_strLog, 500, "%s:卖单下单 限价单", t_strLogFuncName);
            LOG4CPLUS_INFO(m_objLogger, t_strLog);
            m_objOrder.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
        }
    }
    else if (in_nOrderTypeFlag == ORDER_AGAINSTPRICE && m_objOrder.OrderPriceType == THOST_FTDC_OPT_LimitPrice)
    {
        if (m_objOrder.Direction == THOST_FTDC_D_Buy)
        {
            sprintf_s(t_strLog, 500, "%s:买单下单 对价单", t_strLogFuncName);
            LOG4CPLUS_INFO(m_objLogger, t_strLog);
            m_objOrder.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
            // 20180813 niuchao 加速不取行情 m_objOrder.LimitPrice = m_LatestPrice.AskPrice1;
        }
        else if (m_objOrder.Direction == THOST_FTDC_D_Sell)
        {
            sprintf_s(t_strLog, 500, "%s:卖单下单 对价单", t_strLogFuncName);
            LOG4CPLUS_INFO(m_objLogger, t_strLog);
            m_objOrder.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
            // 20180813 niuchao 加速不取行情 m_objOrder.LimitPrice = m_LatestPrice.BidPrice1;
        }
    }
    if (in_plOrderRef == NULL)
    {
        sprintf_s(m_objOrder.OrderRef, sizeof(m_objOrder.OrderRef), "%ld", atol(m_nOrderRef) + 1);
    }
    else
    {
        sprintf_s(m_objOrder.OrderRef, sizeof(m_objOrder.OrderRef), "%ld", ((*in_plOrderRef <= atol(m_nOrderRef)) ? (atol(m_nOrderRef) + 1) : *in_plOrderRef));
        *in_plOrderRef = atol(m_objOrder.OrderRef);
    }
    strcpy_s(m_nOrderRef, sizeof(m_nOrderRef), m_objOrder.OrderRef);
    sprintf_s(t_strLog, 500, "%s:%s-ORDERREF:%s", t_strLogFuncName, m_objOrder.InstrumentID, m_nOrderRef);
    LOG4CPLUS_INFO(m_objLogger, t_strLog);
    // 报单
    m_pUserApi->ReqOrderInsert(&m_objOrder, m_nRequestID++);
    setStatus(_TradeAPI_STATUS_Initiated);
    return 0;
}

/// 撤单 in_OrderSysID为委托号,需要通过查询m_vOrderList获得
int axapi::TradeAPI::MyCancelOrder(APINamespace TThostFtdcOrderSysIDType in_OrderSysID)
{
    char* t_strLogFuncName = "TradeAPI::MyCancelOrder";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    APINamespace CThostFtdcInputOrderActionField t_objInputOrderAction;
    memset(&t_objInputOrderAction, 0, sizeof(t_objInputOrderAction));

    // sqlite版本
#ifdef SQLITE3DATA
    try {
        std::stringstream t_strQrySQL;
        t_strQrySQL << "select BrokerID,InvestorID,OrderRef,RequestID,FrontID,SessionID,ExchangeID,OrderSysID,UserID,InstrumentID from cust_order where OrderSysID = ";
        t_strQrySQL << in_OrderSysID;
        t_strQrySQL << " and OrderStatus not in (";
        t_strQrySQL << THOST_FTDC_OST_Canceled << ", " << THOST_FTDC_OST_AllTraded << ");";
        std::string t_tmpstr = t_strQrySQL.str();
        const char* t_QrySQL = t_tmpstr.c_str();
        sprintf_s(t_strLog, 500, "%s:%s", t_strLogFuncName, t_QrySQL);
        LOG4CPLUS_DEBUG(m_objLogger, t_strLog);

        CppSQLite3Query q = m_objDB.execQuery(t_QrySQL);

        //存在则撤单,不存在则返回错误
        if (!q.eof())
        {
            strcpy_s(t_objInputOrderAction.BrokerID, sizeof(t_objInputOrderAction.BrokerID), q.fieldValue(0));
            strcpy_s(t_objInputOrderAction.InvestorID, sizeof(t_objInputOrderAction.InvestorID), q.fieldValue(1));
            strcpy_s(t_objInputOrderAction.OrderRef, sizeof(t_objInputOrderAction.OrderRef), q.fieldValue(2));
            t_objInputOrderAction.RequestID = atoi(q.fieldValue(3));
            t_objInputOrderAction.FrontID = atoi(q.fieldValue(4));
            t_objInputOrderAction.SessionID = atoi(q.fieldValue(5));
            strcpy_s(t_objInputOrderAction.ExchangeID, sizeof(t_objInputOrderAction.ExchangeID), q.fieldValue(6));
            strcpy_s(t_objInputOrderAction.OrderSysID, sizeof(t_objInputOrderAction.OrderSysID), q.fieldValue(7));
            strcpy_s(t_objInputOrderAction.UserID, sizeof(t_objInputOrderAction.UserID), q.fieldValue(8));
            strcpy_s(t_objInputOrderAction.InstrumentID, sizeof(t_objInputOrderAction.InstrumentID), q.fieldValue(9));
            t_objInputOrderAction.ActionFlag = THOST_FTDC_AF_Delete;
            t_objInputOrderAction.OrderActionRef = m_nRequestID;
            m_pUserApi->ReqOrderAction(&t_objInputOrderAction, m_nRequestID++);
        }
        else
        {
            sprintf_s(t_strLog, 500, "%s:OrderSysID-%d无法撤单,非法状态或不存在", t_strLogFuncName, in_OrderSysID);
            LOG4CPLUS_INFO(m_objLogger, t_strLog);
            return -100;
        }
    }
    catch (CppSQLite3Exception& e)
    {
        sprintf_s(t_strLog, 500, "%s:CppSQLite3错误|%d|%s", t_strLogFuncName, e.errorCode(), e.errorMessage());
        LOG4CPLUS_FATAL(m_objLogger, t_strLog);
        return -200;
    }
#endif SQLITE3DATA
    // 内存版本
#ifdef MEMORYDATA
    try
    {
        int t_nOrderSize = sizeOrderList();
        for (int i = 0; i < t_nOrderSize; i++)
        {
            if (strcmp(m_vOrderList[i].OrderSysID, in_OrderSysID) == 0)
            {
                strcpy_s(t_objInputOrderAction.BrokerID, sizeof(t_objInputOrderAction.BrokerID), m_vOrderList[i].BrokerID);
                strcpy_s(t_objInputOrderAction.InvestorID, sizeof(t_objInputOrderAction.InvestorID), m_vOrderList[i].InvestorID);
                t_objInputOrderAction.OrderActionRef = m_nRequestID; //m_vOrderList[t_nOrderSize].OrderRef;
                strcpy_s(t_objInputOrderAction.OrderRef, sizeof(t_objInputOrderAction.OrderRef), m_vOrderList[i].OrderRef);
                t_objInputOrderAction.RequestID = m_vOrderList[i].RequestID;
                t_objInputOrderAction.FrontID = m_vOrderList[i].FrontID;
                t_objInputOrderAction.SessionID = m_vOrderList[i].SessionID;
                strcpy_s(t_objInputOrderAction.ExchangeID, sizeof(t_objInputOrderAction.ExchangeID), m_vOrderList[i].ExchangeID);
                strcpy_s(t_objInputOrderAction.OrderSysID, sizeof(t_objInputOrderAction.OrderSysID), m_vOrderList[i].OrderSysID);
                t_objInputOrderAction.ActionFlag = THOST_FTDC_AF_Delete;
                t_objInputOrderAction.LimitPrice = m_vOrderList[i].LimitPrice;
                t_objInputOrderAction.VolumeChange = m_vOrderList[i].VolumeTotal;
                strcpy_s(t_objInputOrderAction.UserID, sizeof(t_objInputOrderAction.UserID), m_vOrderList[i].UserID);
                strcpy_s(t_objInputOrderAction.InstrumentID, sizeof(t_objInputOrderAction.InstrumentID), m_vOrderList[i].InstrumentID);
                break;
            }
        }
        if (t_objInputOrderAction.OrderSysID[0] == '\0')
        {
            sprintf_s(t_strLog, 500, "%s:OrderSysID-%s无法撤单,非法状态或不存在", t_strLogFuncName, in_OrderSysID);
            LOG4CPLUS_INFO(m_objLogger, t_strLog);
            return -100;
        }
        else
        {
            m_pUserApi->ReqOrderAction(&t_objInputOrderAction, m_nRequestID++);
            sprintf_s(t_strLog, 500, "%s:OrderSysID-%s撤单指令已发出", t_strLogFuncName, in_OrderSysID);
            LOG4CPLUS_INFO(m_objLogger, t_strLog);
            return 0;
        }
    }
    catch (std::exception e)
    {
        sprintf_s(t_strLog, 500, "%s:OrderSysID-%s异常退出", t_strLogFuncName, in_OrderSysID);
        LOG4CPLUS_WARN(m_objLogger, t_strLog);
        return -200;
    }
#endif MEMORYDATA
    return 0;
}

void axapi::TradeAPI::OnFrontConnected()
{
    char* t_strLogFuncName = "TradeAPI::OnFrontConnected";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s:Head", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);
    sprintf_s(t_strLog, 500, "%s:BrokerID:%s; userid:%s; password:%s;", t_strLogFuncName, m_strBrokerID, m_strUserID, m_strPassword);
    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);

    APINamespace CThostFtdcReqUserLoginField t_reqUserLogin;
    memset(&t_reqUserLogin, 0, sizeof(t_reqUserLogin));
    // set BrokerID
    strcpy_s(t_reqUserLogin.BrokerID, sizeof(t_reqUserLogin.BrokerID), m_strBrokerID);
    // set user id
    strcpy_s(t_reqUserLogin.UserID, sizeof(t_reqUserLogin.UserID), m_strUserID);
    // set password
    strcpy_s(t_reqUserLogin.Password, sizeof(t_reqUserLogin.Password), m_strPassword);
    // send the login request
    m_pUserApi->ReqUserLogin(&t_reqUserLogin, m_nRequestID++);
}

//When the connection between client and the CTP server	disconnected,the follwing function will be called.
void axapi::TradeAPI::OnFrontDisconnected(int nReason)
{
    //  In this  case,  API  willreconnect,the  client  application can ignore this.
    char* t_strLogFuncName = "TradeAPI::OnFrontDisconnected";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    LOG4CPLUS_WARN(m_objLogger, t_strLog);
}

///合约交易状态通知
void axapi::TradeAPI::OnRtnInstrumentStatus(APINamespace CThostFtdcInstrumentStatusField *pInstrumentStatus)
{
    char* t_strLogFuncName = "TradeAPI::OnRtnInstrumentStatus";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    if (pInstrumentStatus != NULL)
    {
        sprintf_s(t_strLog, 500, "%s:%s|%s|%s|%s|%c|%d|%s|%c", t_strLogFuncName,
            pInstrumentStatus->ExchangeID, pInstrumentStatus->ExchangeInstID,
            pInstrumentStatus->SettlementGroupID, pInstrumentStatus->InstrumentID, pInstrumentStatus->InstrumentStatus,
            pInstrumentStatus->TradingSegmentSN, pInstrumentStatus->EnterTime, pInstrumentStatus->EnterReason);
        LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
        insertorUpdateInstrumentStatus(pInstrumentStatus);
    }
}

///报单录入错误回报
void axapi::TradeAPI::OnErrRtnOrderInsert(APINamespace CThostFtdcInputOrderField *pInputOrder, APINamespace CThostFtdcRspInfoField *pRspInfo)
{
    char* t_strLogFuncName = "TradeAPI::OnErrRtnOrderInsert";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    LOG4CPLUS_WARN(m_objLogger, t_strLog);
}

///报单操作错误回报
void axapi::TradeAPI::OnErrRtnOrderAction(APINamespace CThostFtdcOrderActionField *pOrderAction, APINamespace CThostFtdcRspInfoField *pRspInfo)
{
    char* t_strLogFuncName = "TradeAPI::OnErrRtnOrderAction";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    LOG4CPLUS_WARN(m_objLogger, t_strLog);
}

// After receiving the login request from the client,the CTP server will send the following response to notify the client whether the login success or not.
void axapi::TradeAPI::OnRspUserLogin(APINamespace CThostFtdcRspUserLoginField *pRspUserLogin, APINamespace CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    char* t_strLogFuncName = "TradeAPI::OnRspUserLogin";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    if (pRspUserLogin != NULL)
    {
        sprintf_s(t_strLog, 500, "%s:%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%d|%d|%s|", t_strLogFuncName,
            pRspUserLogin->BrokerID,					// 经纪公司代码
            pRspUserLogin->UserID,						// 用户代码
            pRspUserLogin->TradingDay,					// 交易日
            pRspUserLogin->SystemName,					// 交易系统名称
            pRspUserLogin->LoginTime,					// 登陆成功时间
            pRspUserLogin->SHFETime,					// 上期所时间
            pRspUserLogin->DCETime,						// 大商所时间
            pRspUserLogin->CZCETime,					// 郑商所时间
            pRspUserLogin->FFEXTime,					// 中金所时间
            pRspUserLogin->INETime,                     // 能源所时间
            pRspUserLogin->FrontID,						// frond id
            pRspUserLogin->SessionID,					// session id
            pRspUserLogin->MaxOrderRef					// 最大报单引用
        );
        LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
        strcpy_s(m_nOrderRef, sizeof(m_nOrderRef), pRspUserLogin->MaxOrderRef);
        strcpy_s(m_strTradingDay, sizeof(m_strTradingDay), pRspUserLogin->TradingDay);
    }
    if (pRspInfo != NULL)
    {
        sprintf_s(t_strLog, 500, "%s:ErrorCode=[%d], ErrorMsg=[%s]", t_strLogFuncName, pRspInfo->ErrorID, pRspInfo->ErrorMsg);
    }
    else
    {
        sprintf_s(t_strLog, 500, "%s:ErrorCode=null, ErrorMsg=NULL", t_strLogFuncName);
    }
    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
    sprintf_s(t_strLog, 500, "%s:RequestID=[%d], Chain=[%d]", t_strLogFuncName, nRequestID, bIsLast);
    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);

    if (pRspInfo->ErrorID != 0)
    {
        // in case any login failure, the client should handle this error.
        sprintf_s(t_strLog, 500, "%s:Failed to login, errorcode=%d errormsg=%s requestid=%d chain=%d", t_strLogFuncName, pRspInfo->ErrorID, pRspInfo->ErrorMsg, nRequestID, bIsLast);
        LOG4CPLUS_WARN(m_objLogger, t_strLog);
        setStatus(_TradeAPI_STATUS_LinkError);
    }
    else if (pRspInfo->ErrorID == 0)
    {
        sprintf_s(t_strLog, 500, "%s:Success to login, errorcode=%d errormsg=%s requestid=%d chain=%d", t_strLogFuncName, pRspInfo->ErrorID, pRspInfo->ErrorMsg, nRequestID, bIsLast);
        LOG4CPLUS_INFO(m_objLogger, t_strLog);
        setStatus(_TradeAPI_STATUS_Initiated);
    }
    SetEvent(m_hInitEvent);
}

// investor response
void axapi::TradeAPI::OnRspQryInvestor(APINamespace CThostFtdcInvestorField *pInvestor, APINamespace CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    char* t_strLogFuncName = "TradeAPI::OnRspQryInvestor";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s:Head", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    if (NULL != pInvestor)
    {
        sprintf_s(t_strLog, 500, "%s:%s|%s|%s|%s|%s|%s|%c|%s|%s|", t_strLogFuncName,
            pInvestor->InvestorID,						// 投资者代码
            pInvestor->InvestorName,					// 投资者名称
            pInvestor->IdentifiedCardNo,				// 证件号码
            pInvestor->Telephone,						// 联系电话
            pInvestor->Address,							// 通讯地址
            pInvestor->InvestorGroupID,					// 投资者分组代码
            pInvestor->IdentifiedCardType,				// 证件类型
            pInvestor->Mobile,							// 手机
            pInvestor->OpenDate);						// 开户日期
        LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
    }
    if (pRspInfo != NULL)
    {
        sprintf_s(t_strLog, 500, "%s:ErrorCode=[%d], ErrorMsg=[%s]", t_strLogFuncName, pRspInfo->ErrorID, pRspInfo->ErrorMsg);
    }
    else
    {
        sprintf_s(t_strLog, 500, "%s:ErrorCode=null, ErrorMsg=NULL", t_strLogFuncName);
    }
    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
    sprintf_s(t_strLog, 500, "%s:RequestID=[%d], Chain=[%d]", t_strLogFuncName, nRequestID, bIsLast);
    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);

    if (bIsLast == true)
    {
        setCompletedQueryRequestID(nRequestID);
        sprintf_s(t_strLog, 500, "%s:查询客户信息完成,ID为：%d", t_strLogFuncName, nRequestID);
        LOG4CPLUS_INFO(m_objLogger, t_strLog);
    }
}

// tradeaccount response
void axapi::TradeAPI::OnRspQryTradingAccount(APINamespace CThostFtdcTradingAccountField *pTradingAccount, APINamespace CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    char* t_strLogFuncName = "TradeAPI::OnRspQryTradingAccount";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    if (NULL != pTradingAccount)
    {
        sprintf_s(t_strLog, 500, "%s:%s|%.04f|%.04f|%.04f|%.04f|%.04f|%.04f|%.04f|%.04f|%.04f|%.04f|%.04f|", t_strLogFuncName,
            pTradingAccount->AccountID,					// 账号
            pTradingAccount->PreBalance,				// 上次结算准备金
            pTradingAccount->Available,					// 可用资金
            pTradingAccount->Commission,				// 手续费
            pTradingAccount->PositionProfit,			// 持仓盈亏
            pTradingAccount->CloseProfit,				// 平仓盈亏
            pTradingAccount->FrozenCommission,			// 冻结的手续费
            pTradingAccount->FrozenCash,				// 冻结的资金
            pTradingAccount->CurrMargin,				// 当前保证金总额
            pTradingAccount->ExchangeMargin,			// 交易所保证金
            pTradingAccount->Mortgage,					// 质押金额
            pTradingAccount->Credit);					// 信用额度
        LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
        insertorUpdateFund(pTradingAccount);
    }
    if (pRspInfo != NULL)
    {
        sprintf_s(t_strLog, 500, "%s:ErrorCode=[%d], ErrorMsg=[%s]", t_strLogFuncName, pRspInfo->ErrorID, pRspInfo->ErrorMsg);
    }
    else
    {
        sprintf_s(t_strLog, 500, "%s:ErrorCode=null, ErrorMsg=NULL", t_strLogFuncName);
    }
    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
    sprintf_s(t_strLog, 500, "%s:RequestID=[%d], Chain=[%d]", t_strLogFuncName, nRequestID, bIsLast);
    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);

    if (bIsLast == true)
    {
        setCompletedQueryRequestID(nRequestID);
        sprintf_s(t_strLog, 500, "%s:查询资金完成,ID为：%d", t_strLogFuncName, nRequestID);
        LOG4CPLUS_INFO(m_objLogger, t_strLog);
    }
}

// RspQryExchange
void axapi::TradeAPI::OnRspQryExchange(APINamespace CThostFtdcExchangeField *pExchange, APINamespace CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    char* t_strLogFuncName = "TradeAPI::OnRspQryExchange";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    if (NULL != pExchange)
    {
        sprintf_s(t_strLog, 500, "%s:%s|%s", t_strLogFuncName,
            pExchange->ExchangeID,					// 交易所代码
            pExchange->ExchangeName);				// 交易所名称
        LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
    }
    if (pRspInfo != NULL)
    {
        sprintf_s(t_strLog, 500, "%s:ErrorCode=[%d], ErrorMsg=[%s]", t_strLogFuncName, pRspInfo->ErrorID, pRspInfo->ErrorMsg);
    }
    else
    {
        sprintf_s(t_strLog, 500, "%s:ErrorCode=null, ErrorMsg=NULL", t_strLogFuncName);
    }
    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
    sprintf_s(t_strLog, 500, "%s:RequestID=[%d], Chain=[%d]", t_strLogFuncName, nRequestID, bIsLast);
    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);

    if (bIsLast == true)
    {
        setCompletedQueryRequestID(nRequestID);
        sprintf_s(t_strLog, 500, "%s:查询交易所完成,ID为：%d", t_strLogFuncName, nRequestID);
        LOG4CPLUS_INFO(m_objLogger, t_strLog);
    }
}

// RspQryInstrument
void axapi::TradeAPI::OnRspQryInstrument(APINamespace CThostFtdcInstrumentField *pInstrument, APINamespace CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    char* t_strLogFuncName = "TradeAPI::OnRspQryInstrument";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    if (NULL != pInstrument)
    {
        sprintf_s(t_strLog, 500, "%s:%s|%s|%s|%d|%s|%s|%.04f|%d|", t_strLogFuncName,
            pInstrument->ExchangeID,							// 交易所代码
            pInstrument->InstrumentID,							// 合约代码
            pInstrument->InstrumentName,						// 合约名称
            pInstrument->VolumeMultiple,						// 合约数量乘数
            pInstrument->ExpireDate,							// 到期日
            pInstrument->ProductID,								// 产品代码
            pInstrument->PriceTick,								// 最小变动价位
            nRequestID);
        LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
        /*
        被优化
        CThostFtdcInstrumentField t_objInstrument;
        memset(&t_objInstrument, 0, sizeof(t_objInstrument));
        strcpy_s(t_objInstrument.ExchangeID,     sizeof(t_objInstrument.ExchangeID),     pInstrument->ExchangeID);
        strcpy_s(t_objInstrument.InstrumentID,   sizeof(t_objInstrument.InstrumentID),   pInstrument->InstrumentID);
        strcpy_s(t_objInstrument.InstrumentName, sizeof(t_objInstrument.InstrumentName), pInstrument->InstrumentName);
        strcpy_s(t_objInstrument.ExpireDate,     sizeof(t_objInstrument.ExpireDate),     pInstrument->ExpireDate);
        t_objInstrument.VolumeMultiple = pInstrument->VolumeMultiple;
        t_objInstrument.PriceTick = pInstrument->PriceTick;
        */
        insertorUpdateInstrument(pInstrument);
    }
    if (pRspInfo != NULL)
    {
        sprintf_s(t_strLog, 500, "%s:ErrorCode=[%d], ErrorMsg=[%s]", t_strLogFuncName, pRspInfo->ErrorID, pRspInfo->ErrorMsg);
    }
    else
    {
        sprintf_s(t_strLog, 500, "%s:ErrorCode=null, ErrorMsg=NULL", t_strLogFuncName);
    }
    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
    sprintf_s(t_strLog, 500, "%s:RequestID=[%d], Chain=[%d]", t_strLogFuncName, nRequestID, bIsLast);
    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);

    if (bIsLast == true)
    {
        setCompletedQueryRequestID(nRequestID);
        sprintf_s(t_strLog, 500, "%s:查询合约信息完成,ID为：%d", t_strLogFuncName, nRequestID);
        LOG4CPLUS_INFO(m_objLogger, t_strLog);
    }
}

// QryInvestorPositionDetail response
void axapi::TradeAPI::OnRspQryInvestorPositionDetail(APINamespace CThostFtdcInvestorPositionDetailField *pInvestorPositionDetail, APINamespace CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    char* t_strLogFuncName = "TradeAPI::OnRspQryInvestorPositionDetail";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    if (NULL != pInvestorPositionDetail)
    {
        sprintf_s(t_strLog, 500, "%s:%s|%s|%s|%s|%s|%c|%c|%d|%.04f|%.04f|%.04f|", t_strLogFuncName,
            pInvestorPositionDetail->TradingDay,			// 交易日
            pInvestorPositionDetail->OpenDate,				// 开仓日期
            pInvestorPositionDetail->TradeID,				// 成交编号
            pInvestorPositionDetail->InvestorID,			// 投资者代码
            pInvestorPositionDetail->ExchangeID,			// 交易所代码
            pInvestorPositionDetail->Direction,				// 买卖标志
            pInvestorPositionDetail->HedgeFlag,				// 投保标志
            pInvestorPositionDetail->Volume,				// 数量
            pInvestorPositionDetail->OpenPrice,				// 开仓价格
            pInvestorPositionDetail->Margin,				// 投资者保证金
            pInvestorPositionDetail->ExchMargin				// 交易所保证金
        );
        LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
        insertorUpdateSTKHoldDetail(pInvestorPositionDetail);
    }
    if (pRspInfo != NULL)
    {
        sprintf_s(t_strLog, 500, "%s:ErrorCode=[%d], ErrorMsg=[%s]", t_strLogFuncName, pRspInfo->ErrorID, pRspInfo->ErrorMsg);
    }
    else
    {
        sprintf_s(t_strLog, 500, "%s:ErrorCode=null, ErrorMsg=NULL", t_strLogFuncName);
    }
    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
    sprintf_s(t_strLog, 500, "%s:RequestID=[%d], Chain=[%d]", t_strLogFuncName, nRequestID, bIsLast);
    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);

    if (bIsLast == true)
    {
        setCompletedQueryRequestID(nRequestID);
        sprintf_s(t_strLog, 500, "%s:查询持仓明细完成,ID为：%d", t_strLogFuncName, nRequestID);
        LOG4CPLUS_INFO(m_objLogger, t_strLog);
    }
}

// QryInstrumentMarginRate response
void axapi::TradeAPI::OnRspQryInstrumentMarginRate(APINamespace CThostFtdcInstrumentMarginRateField *pInstrumentMarginRate, APINamespace CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    char* t_strLogFuncName = "TradeAPI::OnRspQryInstrumentMarginRate";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    if (NULL != pInstrumentMarginRate)
    {
        sprintf_s(t_strLog, 500, "%s:%s|%s|%.04f|%.04f|%.04f|%.04f|", t_strLogFuncName,
            pInstrumentMarginRate->InvestorID,						// 投资者代码
            pInstrumentMarginRate->InstrumentID,					// 合约代码
            pInstrumentMarginRate->LongMarginRatioByMoney,			// 多头保证金率
            pInstrumentMarginRate->LongMarginRatioByVolume,			// 多头保证金费
            pInstrumentMarginRate->ShortMarginRatioByMoney,			// 空头保证金率
            pInstrumentMarginRate->ShortMarginRatioByVolume);		// 空头保证金费
        LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
    }
    if (pRspInfo != NULL)
    {
        sprintf_s(t_strLog, 500, "%s:ErrorCode=[%d], ErrorMsg=[%s]", t_strLogFuncName, pRspInfo->ErrorID, pRspInfo->ErrorMsg);
    }
    else
    {
        sprintf_s(t_strLog, 500, "%s:ErrorCode=null, ErrorMsg=NULL", t_strLogFuncName);
    }
    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
    sprintf_s(t_strLog, 500, "%s:RequestID=[%d], Chain=[%d]", t_strLogFuncName, nRequestID, bIsLast);
    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);

    if (bIsLast == true)
    {
        setCompletedQueryRequestID(nRequestID);
        sprintf_s(t_strLog, 500, "%s:查询保证金率完成,ID为：%d", t_strLogFuncName, nRequestID);
        LOG4CPLUS_INFO(m_objLogger, t_strLog);
    }
}

// QryInstrumentCommissionRate response
void axapi::TradeAPI::OnRspQryInstrumentCommissionRate(APINamespace CThostFtdcInstrumentCommissionRateField *pInstrumentCommissionRate, APINamespace CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    char* t_strLogFuncName = "TradeAPI::OnRspQryInstrumentCommissionRate";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    if (NULL != pInstrumentCommissionRate)
    {
        sprintf_s(t_strLog, 500, "%s:%s|%s|%.04f|%.04f|%.04f|%.04f|%.04f|%.04f|", t_strLogFuncName,
            pInstrumentCommissionRate->InvestorID,						// 投资者代码
            pInstrumentCommissionRate->InstrumentID,					// 合约代码
            pInstrumentCommissionRate->OpenRatioByMoney,				// 开仓手续费率
            pInstrumentCommissionRate->OpenRatioByVolume,				// 开仓手续费
            pInstrumentCommissionRate->CloseRatioByMoney,				// 平仓手续费率
            pInstrumentCommissionRate->CloseRatioByVolume,				// 平仓手续费
            pInstrumentCommissionRate->CloseTodayRatioByMoney,			// 平今手续费率
            pInstrumentCommissionRate->CloseTodayRatioByVolume);		// 平今手续费
        LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
    }
    if (pRspInfo != NULL)
    {
        sprintf_s(t_strLog, 500, "%s:ErrorCode=[%d], ErrorMsg=[%s]", t_strLogFuncName, pRspInfo->ErrorID, pRspInfo->ErrorMsg);
    }
    else
    {
        sprintf_s(t_strLog, 500, "%s:ErrorCode=null, ErrorMsg=NULL", t_strLogFuncName);
    }
    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
    sprintf_s(t_strLog, 500, "%s:RequestID=[%d], Chain=[%d]", t_strLogFuncName, nRequestID, bIsLast);
    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);

    if (bIsLast == true)
    {
        setCompletedQueryRequestID(nRequestID);
        sprintf_s(t_strLog, 500, "%s:查询手续费率完成,ID为：%d", t_strLogFuncName, nRequestID);
        LOG4CPLUS_INFO(m_objLogger, t_strLog);
    }
}

// output the DepthMarketData result 
void axapi::TradeAPI::OnRspQryDepthMarketData(APINamespace CThostFtdcDepthMarketDataField *pDepthMarketData, APINamespace CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    char* t_strLogFuncName = "TradeAPI::OnRspQryDepthMarketData";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    if (pDepthMarketData != NULL)
    {
        //char t_strMarketData[2000];
        //sprintf_s(t_strLog, 500, "%s:\
        //    %s|%s|%.04f|%.04f|%.04f|%.04f|%.04f|%d|%.04f|%.04f|%.04f|%d|%d|%.04f|%.04f|%.04f|%.04f|%.04f|%s|%d|%.04f|%d|%.04f|%d|%.04f|%d|%.04f|%d|%.04f|%d|%.04f|%d|%.04f|%d|%.04f|",
        //    t_strLogFuncName,
        //    pDepthMarketData->ExchangeID,					// 交易所代码
        //    pDepthMarketData->InstrumentID,					// 合约代码
        //    pDepthMarketData->PreClosePrice,				// 昨收盘
        //    pDepthMarketData->OpenPrice,					// 今开盘
        //    pDepthMarketData->HighestPrice,					// 最高价
        //    pDepthMarketData->LowestPrice,					// 最低价
        //    pDepthMarketData->LastPrice,					// 最新价
        //    pDepthMarketData->Volume,						// 数量
        //    pDepthMarketData->Turnover,						// 成交金额
        //    pDepthMarketData->BidPrice1,					// 申买价一
        //    pDepthMarketData->AskPrice1,					// 申卖价一
        //    pDepthMarketData->BidVolume1,					// 申买量一
        //    pDepthMarketData->AskVolume1,					// 申卖量一
        //    pDepthMarketData->UpperLimitPrice,				// 涨停板价
        //    pDepthMarketData->LowerLimitPrice,				// 跌停板价
        //    pDepthMarketData->PreSettlementPrice,			// 上次结算价
        //    pDepthMarketData->SettlementPrice,				// 本次结算价
        //    pDepthMarketData->OpenInterest,					// 持仓量
        //    pDepthMarketData->TradingDay,					// 交易日
        //    pDepthMarketData->BidVolume2,					// 申买量二
        //    pDepthMarketData->BidPrice2,					// 申买价二
        //    pDepthMarketData->BidVolume3,					// 申买量三
        //    pDepthMarketData->BidPrice3,					// 申买价三
        //    pDepthMarketData->BidVolume4,					// 申买量四
        //    pDepthMarketData->BidPrice4,					// 申买价四
        //    pDepthMarketData->BidVolume5,					// 申买量五
        //    pDepthMarketData->BidPrice5,					// 申买价五
        //    pDepthMarketData->AskVolume2,					// 申卖量二
        //    pDepthMarketData->AskPrice2,					// 申卖价二
        //    pDepthMarketData->AskVolume3,					// 申卖量三
        //    pDepthMarketData->AskPrice3,					// 申卖价三
        //    pDepthMarketData->AskVolume4,					// 申卖量四
        //    pDepthMarketData->AskPrice4,					// 申卖价四
        //    pDepthMarketData->AskVolume5,					// 申卖量五
        //    pDepthMarketData->AskPrice5						// 申卖价五
        //    );
        //LOG4CPLUS_DEBUG(m_objLogger,t_strMarketData);

        memset(&m_LatestPrice, 0, sizeof(m_LatestPrice));
        strcpy_s(m_LatestPrice.ExchangeID, sizeof(m_LatestPrice.ExchangeID), pDepthMarketData->ExchangeID);
        strcpy_s(m_LatestPrice.InstrumentID, sizeof(m_LatestPrice.InstrumentID), pDepthMarketData->InstrumentID);
        m_LatestPrice.PreClosePrice = pDepthMarketData->PreClosePrice;
        m_LatestPrice.OpenPrice = pDepthMarketData->OpenPrice;
        m_LatestPrice.HighestPrice = pDepthMarketData->HighestPrice;
        m_LatestPrice.LowestPrice = pDepthMarketData->LowestPrice;
        m_LatestPrice.LastPrice = pDepthMarketData->LastPrice;
        m_LatestPrice.Volume = pDepthMarketData->Volume;
        m_LatestPrice.Turnover = pDepthMarketData->Turnover;
        m_LatestPrice.BidPrice1 = pDepthMarketData->BidPrice1;
        m_LatestPrice.AskPrice1 = pDepthMarketData->AskPrice1;
        m_LatestPrice.BidVolume1 = pDepthMarketData->BidVolume1;
        m_LatestPrice.AskVolume1 = pDepthMarketData->AskVolume1;
        m_LatestPrice.UpperLimitPrice = pDepthMarketData->UpperLimitPrice;
        m_LatestPrice.LowerLimitPrice = pDepthMarketData->LowerLimitPrice;
        m_LatestPrice.PreSettlementPrice = pDepthMarketData->PreSettlementPrice;
        m_LatestPrice.SettlementPrice = pDepthMarketData->SettlementPrice;
        m_LatestPrice.OpenInterest = pDepthMarketData->OpenInterest;
        strcpy_s(m_LatestPrice.TradingDay, sizeof(m_LatestPrice.TradingDay), pDepthMarketData->TradingDay);
        m_LatestPrice.BidVolume2 = pDepthMarketData->BidVolume2;
        m_LatestPrice.BidPrice2 = pDepthMarketData->BidPrice2;
        m_LatestPrice.BidVolume3 = pDepthMarketData->BidVolume3;
        m_LatestPrice.BidPrice3 = pDepthMarketData->BidPrice3;
        m_LatestPrice.BidVolume4 = pDepthMarketData->BidVolume4;
        m_LatestPrice.BidPrice4 = pDepthMarketData->BidPrice4;
        m_LatestPrice.BidVolume5 = pDepthMarketData->BidVolume5;
        m_LatestPrice.BidPrice5 = pDepthMarketData->BidPrice5;
        m_LatestPrice.AskVolume2 = pDepthMarketData->AskVolume2;
        m_LatestPrice.AskPrice2 = pDepthMarketData->AskPrice2;
        m_LatestPrice.AskVolume3 = pDepthMarketData->AskVolume3;
        m_LatestPrice.AskPrice3 = pDepthMarketData->AskPrice3;
        m_LatestPrice.AskVolume4 = pDepthMarketData->AskVolume4;
        m_LatestPrice.AskPrice4 = pDepthMarketData->AskPrice4;
        m_LatestPrice.AskVolume5 = pDepthMarketData->AskVolume5;
        m_LatestPrice.AskPrice5 = pDepthMarketData->AskPrice5;

        insertorUpdateMarketData(pDepthMarketData);
    }
    if (pRspInfo != NULL)
    {
        sprintf_s(t_strLog, 500, "%s:ErrorCode=[%d], ErrorMsg=[%s]", t_strLogFuncName, pRspInfo->ErrorID, pRspInfo->ErrorMsg);
    }
    else
    {
        sprintf_s(t_strLog, 500, "%s:ErrorCode=null, ErrorMsg=NULL", t_strLogFuncName);
    }
    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
    sprintf_s(t_strLog, 500, "%s:RequestID=[%d], Chain=[%d]", t_strLogFuncName, nRequestID, bIsLast);
    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
    SetEvent(m_hInitEvent);

    if (bIsLast == true)
    {
        setCompletedQueryRequestID(nRequestID);
        sprintf_s(t_strLog, 500, "%s:查询%s行情完成,ID为：%d", t_strLogFuncName, pDepthMarketData->InstrumentID, nRequestID);
        LOG4CPLUS_INFO(m_objLogger, t_strLog);
    }
}

// order insertion response 
void axapi::TradeAPI::OnRspOrderInsert(APINamespace CThostFtdcInputOrderField *pInputOrder, APINamespace CThostFtdcRspInfoField *pRspInfo, int  nRequestID, bool bIsLast)
{
    char* t_strLogFuncName = "TradeAPI::OnRspOrderInsert";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    if (NULL != pInputOrder)
    {
        sprintf_s(t_strLog, 500, "%s:%s", t_strLogFuncName, pInputOrder->OrderRef);
        LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
    }
    if (pRspInfo != NULL)
    {
        sprintf_s(t_strLog, 500, "%s:ErrorCode=[%d], ErrorMsg=[%s]", t_strLogFuncName, pRspInfo->ErrorID, pRspInfo->ErrorMsg);
    }
    else
    {
        sprintf_s(t_strLog, 500, "%s:ErrorCode=null, ErrorMsg=NULL", t_strLogFuncName);
    }
    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
    sprintf_s(t_strLog, 500, "%s:RequestID=[%d], Chain=[%d]", t_strLogFuncName, nRequestID, bIsLast);
    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);

    if (bIsLast == true)
    {
        sprintf_s(t_strLog, "%s:报单%s结果：%d - %s", t_strLogFuncName, pInputOrder->OrderRef, pRspInfo->ErrorID, pRspInfo->ErrorMsg);
        LOG4CPLUS_INFO(m_objLogger, t_strLog);
    };
};

// order insertion return 
void axapi::TradeAPI::OnRtnOrder(APINamespace CThostFtdcOrderField *pOrder)
{
    char* t_strLogFuncName = "TradeAPI::OnRtnOrder";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    if (NULL != pOrder)
    {
        sprintf_s(t_strLog, 500, "%s:%d|%s|OrderSysID:%s|OrderLocalID:%s|%s|%s|%c|%s|%c|%s|%s|%d|%.04f|%d|%d|%s|%s|%s|%c|%c|%c|%c|%.04f|%s|%s|", t_strLogFuncName,
            pOrder->SequenceNo,							// 序号	
            pOrder->InvestorID,							// 客户号
            pOrder->OrderSysID,							// 委托号
            pOrder->OrderLocalID,						// 本地报单编号
            pOrder->ExchangeID,							// 交易所代码
            pOrder->InstrumentID,						// 合约号
            pOrder->OrderStatus,						// 报单状态
            pOrder->StatusMsg,							// 状态信息
            pOrder->Direction,							// 买卖标记
            pOrder->CombOffsetFlag,						// 开平仓标志
            pOrder->CombHedgeFlag,						// 投保标记
            pOrder->VolumeTotalOriginal,				// 委托数量
            pOrder->LimitPrice,							// 委托价格
            pOrder->VolumeTraded,						// 成交数量
            pOrder->VolumeTotal,						// 未成交数量
            pOrder->TradingDay,							// 交割期
            pOrder->InsertTime,							// 委托时间
            pOrder->CancelTime,							// 撤单时间
            pOrder->OrderType,							// 报单类型
            pOrder->OrderSource,						// 报单来源
            pOrder->OrderPriceType,						// 报单价格条件
            pOrder->TimeCondition,						// 有效期类型
            pOrder->StopPrice,							// 止损价
            pOrder->ActiveTime,							// 激活时间
            pOrder->OrderRef							// 报单引用
        );
        LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
        insertorUpdateOrder(pOrder);
    }
    sprintf_s(t_strLog, 500, "%s:RequestID=[%d]", t_strLogFuncName, pOrder->RequestID);
    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
}

///trade return
void axapi::TradeAPI::OnRtnTrade(APINamespace CThostFtdcTradeField *pTrade)
{
    char* t_strLogFuncName = "TradeAPI::OnRtnTrade";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    static int s_nTotalBuy = 0;
    static int s_nTotalSell = 0;

    if (NULL != pTrade)
    {
        if (pTrade->Direction == THOST_FTDC_D_Buy)
        {
            s_nTotalBuy += pTrade->Volume;
        }
        else if (pTrade->Direction == THOST_FTDC_D_Sell)
        {
            s_nTotalSell += pTrade->Volume;
        }
        else
        {
            sprintf_s(t_strLog, 500, "%s:invalid direction:%c", t_strLogFuncName, pTrade->Direction);
            LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
        }

        sprintf_s(t_strLog, 500, "%s:%d|%s|%s|%s|%s|成交|%c|%c|%c|%d|%.04f|%s|%s|%s|%s|s_nTotalBuy=%d|s_nTotalSell=%d|", t_strLogFuncName,
            pTrade->SequenceNo,					// 序号
            pTrade->InvestorID,					// 客户号
            pTrade->ExchangeID,					// 交易所代码
            pTrade->OrderSysID,					// 委托单号
            pTrade->InstrumentID,				// 合约编码
            pTrade->Direction,					// 买卖标记
            pTrade->OffsetFlag,					// 开平仓标志
            pTrade->HedgeFlag,					// 投保标记
            pTrade->Volume,						// 成交数量
            pTrade->Price,						// 成交价格
            pTrade->TradeID,					// 成交号
            pTrade->TradingDay,					// 交割期
            pTrade->TradeTime,					// 成交时间
            pTrade->OrderRef,					// 报单引用
            s_nTotalBuy,
            s_nTotalSell
        );
        LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
        insertorUpdateTrade(pTrade);
    }
}

// the error notification caused by client request
void axapi::TradeAPI::OnRspError(APINamespace CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    char* t_strLogFuncName = "TradeAPI::OnRspError";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    if (pRspInfo != NULL)
    {
        sprintf_s(t_strLog, 500, "%s:ErrorCode=[%d], ErrorMsg=[%s]", t_strLogFuncName, pRspInfo->ErrorID, pRspInfo->ErrorMsg);
    }
    else
    {
        sprintf_s(t_strLog, 500, "%s:ErrorCode=null, ErrorMsg=NULL", t_strLogFuncName);
    }
    LOG4CPLUS_WARN(m_objLogger, t_strLog);
    sprintf_s(t_strLog, 500, "%s:RequestID=[%d], Chain=[%d]", t_strLogFuncName, nRequestID, bIsLast);
    LOG4CPLUS_WARN(m_objLogger, t_strLog);

    // the client should handle the error
    //SetStatus(_TradeAPI_STATUS_UndefinedError);
}

// output the order action result 
void axapi::TradeAPI::OnRspOrderAction(APINamespace CThostFtdcInputOrderActionField *pInputOrderAction, APINamespace CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    char* t_strLogFuncName = "TradeAPI::OnRspOrderAction";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    if (NULL != pInputOrderAction)
    {
        sprintf_s(t_strLog, 500, "%s:%s|OrderSysID:%s|%s|%s|%.04f|", t_strLogFuncName,
            pInputOrderAction->InvestorID,							// 客户号
            pInputOrderAction->OrderSysID,							// 委托号
            pInputOrderAction->ExchangeID,							// 交易所代码
            pInputOrderAction->InstrumentID,						// 合约号
            pInputOrderAction->LimitPrice							// 委托价格
        );
        LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
    }
    if (pRspInfo != NULL)
    {
        sprintf_s(t_strLog, 500, "%s:ErrorCode=[%d], ErrorMsg=[%s]", t_strLogFuncName, pRspInfo->ErrorID, pRspInfo->ErrorMsg);
    }
    else
    {
        sprintf_s(t_strLog, 500, "%s:ErrorCode=null, ErrorMsg=NULL", t_strLogFuncName);
    }
    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
    sprintf_s(t_strLog, 500, "%s:RequestID=[%d], Chain=[%d]", t_strLogFuncName, nRequestID, bIsLast);
    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);

    if (bIsLast == true)
    {
    }
}

// qryorder return
void axapi::TradeAPI::OnRspQryOrder(APINamespace CThostFtdcOrderField *pOrder, APINamespace CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    char* t_strLogFuncName = "TradeAPI::OnRspQryOrder";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    if (pOrder != NULL)
    {
        sprintf_s(t_strLog, 500, "%s:%s", t_strLogFuncName, pOrder->OrderSysID);
        LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
        insertorUpdateOrder(pOrder);
        sprintf_s(t_strLog, 500, "%s:OrderStatus=%c|OrderSysID=%s", t_strLogFuncName, pOrder->OrderStatus, pOrder->OrderSysID);
        LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
    }
    if (pRspInfo != NULL)
    {
        sprintf_s(t_strLog, 500, "%s:ErrorCode=[%d], ErrorMsg=[%s]", t_strLogFuncName, pRspInfo->ErrorID, pRspInfo->ErrorMsg);
    }
    else
    {
        sprintf_s(t_strLog, 500, "%s:ErrorCode=null, ErrorMsg=NULL", t_strLogFuncName);
    }
    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
    sprintf_s(t_strLog, 500, "%s:RequestID=[%d], Chain=[%d]", t_strLogFuncName, nRequestID, bIsLast);
    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);

    if (bIsLast == true)
    {
        setCompletedQueryRequestID(nRequestID);
        sprintf_s(t_strLog, 500, "%s:查询报单完成ID为：%d", t_strLogFuncName, nRequestID);
        LOG4CPLUS_INFO(m_objLogger, t_strLog);
    }
}

// qrytrade return
void axapi::TradeAPI::OnRspQryTrade(APINamespace CThostFtdcTradeField *pTrade, APINamespace CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    char* t_strLogFuncName = "TradeAPI::OnRspQryTrade";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    if (pTrade != NULL)
    {
        sprintf_s(t_strLog, 500, "%s:%s|%s|%s|%s|%c|%c|%c|%d|%.04f|%s|%s|%s|%s|%d", t_strLogFuncName,
            pTrade->InvestorID,							// 客户号
            pTrade->ExchangeID,							// 交易所代码
            pTrade->OrderSysID,							// 报单编号
            pTrade->InstrumentID,						// 合约代码
            pTrade->Direction,							// 买卖标记
            pTrade->OffsetFlag,							// 开平标志
            pTrade->HedgeFlag,							// 投保标记
            pTrade->Volume,								// 成交数量
            pTrade->Price,								// 成交价格
            pTrade->TradeID,							// 成交号
            pTrade->TradeDate,							// 日期
            pTrade->TradingDay,							// 交易日
            pTrade->TradeTime,							// 成交时间
            pTrade->SequenceNo							// 序号
        );
        LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
        insertorUpdateTrade(pTrade);
    }
    if (pRspInfo != NULL)
    {
        sprintf_s(t_strLog, 500, "%s:ErrorCode=[%d], ErrorMsg=[%s]", t_strLogFuncName, pRspInfo->ErrorID, pRspInfo->ErrorMsg);
    }
    else
    {
        sprintf_s(t_strLog, 500, "%s:ErrorCode=null, ErrorMsg=NULL", t_strLogFuncName);
    }
    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
    sprintf_s(t_strLog, 500, "%s:RequestID=[%d], Chain=[%d]", t_strLogFuncName, nRequestID, bIsLast);
    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);

    if (bIsLast == true)
    {
        setCompletedQueryRequestID(nRequestID);
        sprintf_s(t_strLog, 500, "%s:查询成交完成,ID为：%d", t_strLogFuncName, nRequestID);
        LOG4CPLUS_INFO(m_objLogger, t_strLog);
    }
}

// QryInvestorPosition return
void axapi::TradeAPI::OnRspQryInvestorPosition(APINamespace CThostFtdcInvestorPositionField *pInvestorPosition, APINamespace CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    char* t_strLogFuncName = "TradeAPI::OnRspQryInvestorPosition";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    if (pInvestorPosition != NULL)
    {
        sprintf_s(t_strLog, 500, "%s:%s|%d|%d|%.04f|%.04f|%s|%c|%.04f|%c|%s|%.04f|", t_strLogFuncName,
            pInvestorPosition->InvestorID,					// 客户号
            pInvestorPosition->Position,					// 今日总持仓
            pInvestorPosition->TodayPosition,				// 今日现持仓
            pInvestorPosition->PositionCost,				// 持仓成本	
            pInvestorPosition->OpenCost,					// 开仓成本
            pInvestorPosition->InstrumentID,				// 合约代码
            pInvestorPosition->HedgeFlag,					// 投机套保标志
            pInvestorPosition->PositionProfit,				// 持仓盈亏
            pInvestorPosition->PosiDirection,				// 持仓多空方向
            pInvestorPosition->TradingDay,					// 交易日
            pInvestorPosition->UseMargin					// 占用的保证金
        );
        LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
        insertorUpdateSTKHold(pInvestorPosition);
    }
    if (pRspInfo != NULL)
    {
        sprintf_s(t_strLog, 500, "%s:ErrorCode=[%d], ErrorMsg=[%s]", t_strLogFuncName, pRspInfo->ErrorID, pRspInfo->ErrorMsg);
    }
    else
    {
        sprintf_s(t_strLog, 500, "%s:ErrorCode=null, ErrorMsg=NULL", t_strLogFuncName);
    }
    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
    sprintf_s(t_strLog, 500, "%s:RequestID=[%d], Chain=[%d]", t_strLogFuncName, nRequestID, bIsLast);
    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);

    if (bIsLast == true)
    {
        setCompletedQueryRequestID(nRequestID);
        sprintf_s(t_strLog, 500, "%s:查询持仓完成,ID为：%d", t_strLogFuncName, nRequestID);
        LOG4CPLUS_INFO(m_objLogger, t_strLog);
    }
}

// logout return
void axapi::TradeAPI::OnRspUserLogout(APINamespace CThostFtdcUserLogoutField *pUserLogout, APINamespace CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    char* t_strLogFuncName = "TradeAPI::OnRspUserLogout";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    if (NULL != pUserLogout)
    {
        sprintf_s(t_strLog, 500, "%s:%s", pUserLogout->UserID, t_strLogFuncName);
        LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
    }
    if (pRspInfo != NULL)
    {
        sprintf_s(t_strLog, 500, "%s:ErrorCode=[%d], ErrorMsg=[%s]", t_strLogFuncName, pRspInfo->ErrorID, pRspInfo->ErrorMsg);
    }
    else
    {
        sprintf_s(t_strLog, 500, "%s:ErrorCode=null, ErrorMsg=NULL", t_strLogFuncName);
    }
    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
    sprintf_s(t_strLog, 500, "%s:RequestID=[%d], Chain=[%d]", t_strLogFuncName, nRequestID, bIsLast);
    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
}

void axapi::TradeAPI::OnRspQrySettlementInfo(APINamespace CThostFtdcSettlementInfoField *pSettlementInfo, APINamespace CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    char* t_strLogFuncName = "TradeAPI::OnRspQrySettlementInfo";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    if (pSettlementInfo != NULL)
    {
        sprintf_s(t_strLog, 500, "%s:%s|%s|%d|%s|%s", t_strLogFuncName, pSettlementInfo->BrokerID, pSettlementInfo->InvestorID, pSettlementInfo->SettlementID, pSettlementInfo->TradingDay, pSettlementInfo->Content);
        LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
    }
    if (pRspInfo != NULL)
    {
        sprintf_s(t_strLog, 500, "%s:ErrorCode=[%d], ErrorMsg=[%s]", t_strLogFuncName, pRspInfo->ErrorID, pRspInfo->ErrorMsg);
        LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
        sprintf_s(t_strLog, 500, "%s:RequestID=[%d], Chain=[%d]", t_strLogFuncName, nRequestID, bIsLast);
        LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
    }
    if (bIsLast == true)
    {
        setCompletedQueryRequestID(nRequestID);
        sprintf_s(t_strLog, 500, "%s:查询结算信息完成,ID为：%d", t_strLogFuncName, nRequestID);
        LOG4CPLUS_INFO(m_objLogger, t_strLog);
    }
}

void axapi::TradeAPI::OnRspQrySettlementInfoConfirm(APINamespace CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, APINamespace CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    char* t_strLogFuncName = "TradeAPI::OnRspQrySettlementInfoConfirm";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    if (pSettlementInfoConfirm != NULL)
    {
        sprintf_s(t_strLog, 500, "%s:%s|%s|%s|%s", t_strLogFuncName, pSettlementInfoConfirm->BrokerID, pSettlementInfoConfirm->InvestorID, pSettlementInfoConfirm->ConfirmDate, pSettlementInfoConfirm->ConfirmTime);
        LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
    }
    if (pRspInfo != NULL)
    {
        sprintf_s(t_strLog, 500, "%s:ErrorCode=[%d], ErrorMsg=[%s]", t_strLogFuncName, pRspInfo->ErrorID, pRspInfo->ErrorMsg);
        LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
        sprintf_s(t_strLog, 500, "%s:RequestID=[%d], Chain=[%d]", t_strLogFuncName, nRequestID, bIsLast);
        LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
    }
    if (bIsLast == true)
    {
        setCompletedQueryRequestID(nRequestID);
        sprintf_s(t_strLog, 500, "%s:查询结算信息确认完成,ID为：%d", t_strLogFuncName, nRequestID);
        LOG4CPLUS_INFO(m_objLogger, t_strLog);
    }
}

void axapi::TradeAPI::OnRspSettlementInfoConfirm(APINamespace CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, APINamespace CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    char* t_strLogFuncName = "TradeAPI::OnRspSettlementInfoConfirm";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    if (pSettlementInfoConfirm != NULL)
    {
        sprintf_s(t_strLog, 500, "%s:%s|%s|%s|%s", t_strLogFuncName, pSettlementInfoConfirm->BrokerID, pSettlementInfoConfirm->InvestorID, pSettlementInfoConfirm->ConfirmDate, pSettlementInfoConfirm->ConfirmTime);
        LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
    }
    if (pRspInfo != NULL)
    {
        sprintf_s(t_strLog, 500, "%s:ErrorCode=[%d], ErrorMsg=[%s]", t_strLogFuncName, pRspInfo->ErrorID, pRspInfo->ErrorMsg);
        LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
        sprintf_s(t_strLog, 500, "%s:RequestID=[%d], Chain=[%d]", t_strLogFuncName, nRequestID, bIsLast);
        LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
    }
    if (bIsLast == true)
    {
        setCompletedQueryRequestID(nRequestID);
    }
}

///交易所公告通知
#ifdef CTP_TRADEAPI
void axapi::TradeAPI::OnRtnBulletin(APINamespace CThostFtdcBulletinField *pBulletin)
{
    char* t_strLogFuncName = "TradeAPI::OnRtnBulletin";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    LOG4CPLUS_INFO(m_objLogger, t_strLog);

    if (pBulletin != NULL)
    {
        /*sprintf_s(t_strLog, 500, "%s:%s|%s|%d|%d|%s|%c|%s|%s|%s|%s|%s|%s", t_strLogFuncName, pBulletin->ExchangeID, pBulletin->TradingDay,
        pBulletin->BulletinID, pBulletin->SequenceNo, pBulletin->NewsType, pBulletin->NewsUrgency,
        pBulletin->SendTime, pBulletin->Abstract, pBulletin->ComeFrom, pBulletin->Content,
        pBulletin->URLLink, pBulletin->MarketID);
        LOG4CPLUS_DEBUG(m_objLogger,t_strLog);*/
    }
}
#endif CTP_TRADEAPI

///交易通知
#ifdef CTP_TRADEAPI
void axapi::TradeAPI::OnRtnTradingNotice(APINamespace CThostFtdcTradingNoticeInfoField *pTradingNoticeInfo)
{
    char* t_strLogFuncName = "TradeAPI::OnRtnTradingNotice";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    if (pTradingNoticeInfo != NULL)
    {
        sprintf_s(t_strLog, 500, "%s:%s|%s|%s|%s|%d|%d|%s", t_strLogFuncName,
            pTradingNoticeInfo->BrokerID, pTradingNoticeInfo->InvestorID, pTradingNoticeInfo->SendTime,
            pTradingNoticeInfo->FieldContent, pTradingNoticeInfo->SequenceSeries, pTradingNoticeInfo->SequenceNo,
            pTradingNoticeInfo->InvestUnitID);
        LOG4CPLUS_INFO(m_objLogger, t_strLog);
    }
}
#endif CTP_TRADEAPI

///执行宣告通知
void axapi::TradeAPI::OnRtnExecOrder(APINamespace CThostFtdcExecOrderField *pExecOrder)
{
    char* t_strLogFuncName = "TradeAPI::OnRtnExecOrder";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    if (pExecOrder != NULL)
    {
        /*sprintf_s(t_strLog, 500, "%s:%s|%s|%s|%s|%d|%d|%s", t_strLogFuncName,
        pExecOrder->BrokerID, pExecOrder->InvestorID, pExecOrder->SendTime,
        pExecOrder->FieldContent, pExecOrder->SequenceSeries, pExecOrder->SequenceNo,
        pExecOrder->InvestUnitID);
        LOG4CPLUS_DEBUG(m_objLogger,t_strLog);*/
    }
}

/// 处理所有插入或者更新cust_order的操作,ret:0 正常
int axapi::TradeAPI::insertorUpdateOrder(APINamespace CThostFtdcOrderField *pOrder)
{
    char* t_strLogFuncName = "TradeAPI::insertorUpdateOrder";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    // sqlite版本
#ifdef SQLITE3DATA
    try
    {
        std::stringstream t_strQrySQL;
        t_strQrySQL << "select count(*) cnt from cust_order where OrderSysID = '";
        t_strQrySQL << pOrder->OrderSysID << "';";
        std::string t_tmpstr = t_strQrySQL.str();
        const char* t_QrySQL = t_tmpstr.c_str();
        sprintf_s(t_strLog, 500, "%s:%s", t_strLogFuncName, t_QrySQL);
        LOG4CPLUS_DEBUG(m_objLogger, t_strLog);

        CppSQLite3Query q = m_objDB.execQuery(t_QrySQL);

        // 存在报单则更新报单信息,不存在则插入报单,并检验是否需要更新orderref最大值
        if (!q.eof())
        {
            if (strcmp((q.fieldValue(0)), "0") == 0)
            {
                // 不存在报单
                // 先更新最大orderref
                if (atol(m_nOrderRef) < atol(pOrder->OrderLocalID))
                {
                    strcpy_s(m_nOrderRef, sizeof(m_nOrderRef), pOrder->OrderLocalID);
                    sprintf_s(t_strLog, 500, "%s:Local OrderRef updated to %s", t_strLogFuncName, m_nOrderRef);
                    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
                }
                try
                {
                    // 构造insert sql语句
                    std::stringstream t_strIstSQL;
                    t_strIstSQL << "insert into cust_order(";
                    t_strIstSQL << "BrokerID, InvestorID, InstrumentID, UserID, OrderRef, Direction, LimitPrice, VolumeTotalOriginal, VolumeTotal, VolumeTraded, OrderStatus, OrderSysID, OrderLocalID, RequestID, FrontID, SessionID, ExchangeID";
                    t_strIstSQL << ") values (";
                    t_strIstSQL << " '" << pOrder->BrokerID << "'";
                    t_strIstSQL << ", '" << pOrder->InvestorID << "'";
                    t_strIstSQL << ", '" << pOrder->InstrumentID << "'";
                    t_strIstSQL << ", '" << pOrder->UserID << "'";
                    t_strIstSQL << ", '" << pOrder->OrderRef << "'";
                    t_strIstSQL << ", '" << pOrder->Direction << "'";
                    t_strIstSQL << ", " << pOrder->LimitPrice;
                    t_strIstSQL << ", " << pOrder->VolumeTotalOriginal;
                    t_strIstSQL << ", " << pOrder->VolumeTotal;
                    t_strIstSQL << ", " << pOrder->VolumeTraded;
                    t_strIstSQL << ", '" << pOrder->OrderStatus << "'";
                    t_strIstSQL << ", '" << pOrder->OrderSysID << "'";
                    t_strIstSQL << ", '" << pOrder->OrderLocalID << "'";
                    t_strIstSQL << ", " << pOrder->RequestID;
                    t_strIstSQL << ", " << pOrder->FrontID;
                    t_strIstSQL << ", " << pOrder->SessionID;
                    t_strIstSQL << ", '" << pOrder->ExchangeID << "'";
                    t_strIstSQL << " );";
                    t_tmpstr = t_strIstSQL.str();
                    const char* t_IstSQL = t_tmpstr.c_str();
                    sprintf_s(t_strLog, 500, "%s:%s", t_strLogFuncName, t_IstSQL);
                    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
                    sprintf_s(t_strLog, 500, "%s:insert %s|%s|%c|%d", t_strLogFuncName, pOrder->OrderRef, pOrder->InstrumentID, pOrder->Direction, pOrder->VolumeTotal);
                    LOG4CPLUS_INFO(m_objLogger, t_strLog);

                    // 执行insert sql语句
                    int t_res = m_objDB.execDML(t_IstSQL);
                    // 判断处理结果
                    if (t_res != 1)
                    {
                        sprintf_s(t_strLog, 500, "%s:执行insert SQL 出错.", t_strLogFuncName);
                        LOG4CPLUS_ERROR(m_objLogger, t_strLog);
                        return -100;
                    }
                    sprintf_s(t_strLog, 500, "%s:执行insert SQL完成.", t_strLogFuncName);
                    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
                }
                catch (CppSQLite3Exception& e)
                {
                    sprintf_s(t_strLog, 500, "%s:插入cust_order错误|%d|%s", t_strLogFuncName, e.errorCode(), e.errorMessage());
                    LOG4CPLUS_ERROR(m_objLogger, t_strLog);
                    return -100;
                }
            }
            else
            {
                // 存在报单信息
                try
                {
                    // 构造update sql语句
                    std::stringstream t_strUpdSQL;
                    t_strUpdSQL << "update cust_order set";
                    t_strUpdSQL << " OrderStatus=" << "'" << pOrder->OrderStatus << "'";
                    t_strUpdSQL << ", VolumeTotal=" << pOrder->VolumeTotal << "";
                    t_strUpdSQL << ", VolumeTraded=" << pOrder->VolumeTraded << "";
                    t_strUpdSQL << " where OrderSysID=" << "'" << pOrder->OrderSysID << "'";
                    t_strUpdSQL << " ;";
                    t_tmpstr = t_strUpdSQL.str();
                    const char* t_UpdSQL = t_tmpstr.c_str();
                    sprintf_s(t_strLog, 500, "%s:%s", t_strLogFuncName, t_UpdSQL);
                    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
                    sprintf_s(t_strLog, 500, "%s:update %s|%s|%c", t_strLogFuncName, pOrder->OrderRef, pOrder->InstrumentID, pOrder->Direction);
                    LOG4CPLUS_INFO(m_objLogger, t_strLog);

                    // 执行update sql语句
                    int t_res = m_objDB.execDML(t_UpdSQL);
                    // 判断处理结果
                    if (t_res != 1)
                    {
                        sprintf_s(t_strLog, 500, "%s:执行update SQL 出错.", t_strLogFuncName);
                        LOG4CPLUS_ERROR(m_objLogger, t_strLog);
                        return -100;
                    }
                    sprintf_s(t_strLog, 500, "%s:执行update SQL完成.", t_strLogFuncName);
                    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
                }
                catch (CppSQLite3Exception& e)
                {
                    sprintf_s(t_strLog, 500, "%s:更新cust_order错误|%d|%s", t_strLogFuncName, e.errorCode(), e.errorMessage());
                    LOG4CPLUS_ERROR(m_objLogger, t_strLog);
                    return -100;
                }
            }
        }
        else
        {
            sprintf_s(t_strLog, 500, "%s:cust_order查询失败|%s", t_strLogFuncName, pOrder->OrderRef);
            LOG4CPLUS_ERROR(m_objLogger, t_strLog);
            return -100;
        }
    }
    catch (CppSQLite3Exception& e)
    {
        sprintf_s(t_strLog, 500, "%s:CppSQLite3错误|%d|%s", t_strLogFuncName, e.errorCode(), e.errorMessage());
        LOG4CPLUS_FATAL(m_objLogger, t_strLog);
        return -200;
    }
#endif SQLITE3DATA
    // 内存版本
#ifdef MEMORYDATA
    try
    {
        int t_nUpdatePosition = -1;
        for (unsigned int i = 0; i < m_vOrderList.size(); i++)
        {
            if (strcmp(m_vOrderList[i].OrderSysID, pOrder->OrderSysID) == 0)
            {
                t_nUpdatePosition = i;
                sprintf_s(t_strLog, 500, "%s:find recorded order info m_vOrderList[%d]:%s", t_strLogFuncName, t_nUpdatePosition, m_vOrderList[t_nUpdatePosition].OrderSysID);
                LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
                break;
            }
        }
        if (t_nUpdatePosition == -1)
        {
            APINamespace CThostFtdcOrderField t_OrderField;
            memset(&t_OrderField, '\0', sizeof(t_OrderField));
            memcpy_s(&t_OrderField, sizeof(t_OrderField), pOrder, sizeof(APINamespace CThostFtdcOrderField));
            sprintf_s(t_strLog, 500, "%s:%s", t_strLogFuncName, t_OrderField.InstrumentID);
            m_vOrderList.push_back(t_OrderField);
            if (atol(t_OrderField.OrderRef) > atol(m_nOrderRef))
            {
                sprintf_s(m_nOrderRef, sizeof(m_nOrderRef), "%ld", atol(t_OrderField.OrderRef));
            }
            sprintf_s(t_strLog, 500, "%s:insert data(%s) to m_vOrderList[%d]", t_strLogFuncName, m_vOrderList[m_vOrderList.size() - 1].OrderSysID, m_vOrderList.size() - 1);
            LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
        }
        else
        {
            m_vOrderList[t_nUpdatePosition].VolumeTraded = pOrder->VolumeTraded;
            m_vOrderList[t_nUpdatePosition].VolumeTotal = pOrder->VolumeTotal;
            m_vOrderList[t_nUpdatePosition].OrderStatus = pOrder->OrderStatus;
            sprintf_s(t_strLog, 500, "%s:update data(%s) in m_vOrderList[%d]", t_strLogFuncName, m_vOrderList[t_nUpdatePosition].OrderSysID, t_nUpdatePosition);
            LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
        }
    }
    catch (std::exception e)
    {
        sprintf_s(t_strLog, 500, "%s:错误%s", t_strLogFuncName, e.what());
        LOG4CPLUS_ERROR(m_objLogger, t_strLog);
        return -200;
    }
#endif MEMORYDATA
    return 0;
}

/// 处理所有插入或者更新cust_done的操作,ret:0 正常
int axapi::TradeAPI::insertorUpdateTrade(APINamespace CThostFtdcTradeField *pTrade)
{
    char* t_strLogFuncName = "TradeAPI::insertorUpdateTrade";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    // sqlite版本
#ifdef SQLITE3DATA
    try {
        std::stringstream t_strQrySQL;
        t_strQrySQL << "select count(*) cnt from cust_done where TradeID = ";
        t_strQrySQL << pTrade->TradeID << ";";
        std::string t_tmpstr = t_strQrySQL.str();
        const char* t_QrySQL = t_tmpstr.c_str();
        sprintf_s(t_strLog, 500, "%s:%s", t_strLogFuncName, t_QrySQL);
        LOG4CPLUS_DEBUG(m_objLogger, t_strLog);

        CppSQLite3Query q = m_objDB.execQuery(t_QrySQL);

        //存在成交则更新成交信息,不存在则插入成交
        if (!q.eof())
        {
            if (*(q.fieldValue(0)) == '0')
            {
                // 不存在报单
                try
                {
                    // 构造insert sql语句
                    std::stringstream t_strIstSQL;
                    t_strIstSQL << "insert into cust_done(";
                    t_strIstSQL << "BrokerID, InvestorID, InstrumentID, OrderRef, UserID, ExchangeID, TradeID, Direction, OrderSysID, Price, Volume";
                    t_strIstSQL << ") values (";
                    t_strIstSQL << " '" << pTrade->BrokerID << "'";
                    t_strIstSQL << ", '" << pTrade->InvestorID << "'";
                    t_strIstSQL << ", '" << pTrade->InstrumentID << "'";
                    t_strIstSQL << ", '" << pTrade->OrderRef << "'";
                    t_strIstSQL << ", '" << pTrade->UserID << "'";
                    t_strIstSQL << ", '" << pTrade->ExchangeID << "'";
                    t_strIstSQL << ", '" << pTrade->TradeID << "'";
                    t_strIstSQL << ", '" << pTrade->Direction << "'";
                    t_strIstSQL << ", '" << pTrade->OrderSysID << "'";
                    t_strIstSQL << ", '" << pTrade->Price << "'";
                    t_strIstSQL << ", '" << pTrade->Volume << "'";
                    t_strIstSQL << " );";
                    t_tmpstr = t_strIstSQL.str();
                    const char* t_IstSQL = t_tmpstr.c_str();
                    sprintf_s(t_strLog, 500, "%s:%s", t_strLogFuncName, t_IstSQL);
                    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
                    sprintf_s(t_strLog, 499, "%s:insert %s|%s|%c|%d", t_strLogFuncName, pTrade->OrderRef, pTrade->InstrumentID, pTrade->Direction, pTrade->Volume);
                    LOG4CPLUS_INFO(m_objLogger, t_strLog);

                    // 执行insert sql语句
                    int t_res = m_objDB.execDML(t_IstSQL);
                    // 判断处理结果
                    if (t_res != 1)
                    {
                        sprintf_s(t_strLog, 500, "%s:执行insert SQL 出错.", t_strLogFuncName);
                        LOG4CPLUS_ERROR(m_objLogger, t_strLog);
                        return -100;
                    }
                    sprintf_s(t_strLog, 500, "%s:执行insert SQL完成.", t_strLogFuncName);
                    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
                }
                catch (CppSQLite3Exception& e)
                {
                    sprintf_s(t_strLog, 500, "%s:插入cust_done错误|%d|%s", t_strLogFuncName, e.errorCode(), e.errorMessage());
                    LOG4CPLUS_ERROR(m_objLogger, t_strLog);
                    return -100;
                }
            }
            else
            {
                // 存在报单信息
                try
                {
                    // 构造update sql语句
                    std::stringstream t_strUpdSQL;
                    t_strUpdSQL << "update cust_done set";
                    t_strUpdSQL << " Price=" << "'" << pTrade->Price << "'";
                    t_strUpdSQL << ", Volume=" << pTrade->Volume << "";
                    t_strUpdSQL << " where TradeID=" << "'" << pTrade->TradeID << "'";
                    t_strUpdSQL << " ;";
                    t_tmpstr = t_strUpdSQL.str();
                    const char* t_UpdSQL = t_tmpstr.c_str();
                    sprintf_s(t_strLog, 500, "%s:%s", t_strLogFuncName, t_UpdSQL);
                    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
                    sprintf_s(t_strLog, 500, "%s:update %s|%s", t_strLogFuncName, pTrade->OrderRef, pTrade->InstrumentID, pTrade->Direction, pTrade->Volume);
                    LOG4CPLUS_INFO(m_objLogger, t_strLog);

                    // 执行update sql语句
                    int t_res = m_objDB.execDML(t_UpdSQL);
                    // 判断处理结果
                    if (t_res != 1)
                    {
                        sprintf_s(t_strLog, 500, "%s:执行update SQL 出错.", t_strLogFuncName);
                        LOG4CPLUS_ERROR(m_objLogger, t_strLog);
                        return -100;
                    }
                    sprintf_s(t_strLog, 500, "%s:执行update SQL完成.", t_strLogFuncName);
                    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
                }
                catch (CppSQLite3Exception& e)
                {
                    sprintf_s(t_strLog, 500, "%s:更新cust_order错误|%d|%s", t_strLogFuncName, e.errorCode(), e.errorMessage());
                    LOG4CPLUS_ERROR(m_objLogger, t_strLog);
                    return -100;
                }
            }
        }
        else
        {
            sprintf_s(t_strLog, 500, "%s:cust_done查询失败|%s", t_strLogFuncName, pTrade->TradeID);
            LOG4CPLUS_ERROR(m_objLogger, t_strLog);
            return -100;
        }
    }
    catch (CppSQLite3Exception& e)
    {
        sprintf_s(t_strLog, 500, "%s:CppSQLite3错误|%d|%s", t_strLogFuncName, e.errorCode(), e.errorMessage());
        LOG4CPLUS_FATAL(m_objLogger, t_strLog);
        return -200;
    }
#endif SQLITE3DATA
    // 内存版本
#ifdef MEMORYDATA
    try
    {
        int t_nUpdatePosition = -1;
        for (unsigned int i = 0; i < m_vTradeList.size(); i++)
        {
            if (strcmp(m_vTradeList[i].apiTradeField.TradeID, pTrade->TradeID) == 0)
            {
                t_nUpdatePosition = i;
                sprintf_s(t_strLog, 500, "%s:find recorded trade info m_vTradeList[%d]:%s", t_strLogFuncName, t_nUpdatePosition, m_vTradeList[t_nUpdatePosition].apiTradeField.TradeID);
                LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
                break;
            }
        }
        if (t_nUpdatePosition == -1)
        {
            TradeField t_tradefield;
            memset(&t_tradefield, '\0', sizeof(TradeField));
            memcpy_s(&t_tradefield.apiTradeField, sizeof(t_tradefield.apiTradeField), pTrade, sizeof(*pTrade));
            t_tradefield.Volumn = pTrade->Volume;
            t_tradefield.Price = pTrade->Price;
            m_vTradeList.push_back(t_tradefield);
            sprintf_s(t_strLog, 500, "%s:insert data(%s) to m_vTradeList[%d]", t_strLogFuncName, m_vTradeList[m_vTradeList.size() - 1].apiTradeField.TradeID, m_vTradeList.size() - 1);
            LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
        }
        else
        {
            sprintf_s(t_strLog, 500, "%s:update data(%s) in m_vTradeList[%d]", t_strLogFuncName, m_vTradeList[t_nUpdatePosition].apiTradeField.TradeID, t_nUpdatePosition);
            LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
        }
    }
    catch (std::exception e)
    {
        sprintf_s(t_strLog, 500, "%s:错误%s", t_strLogFuncName, e.what());
        LOG4CPLUS_ERROR(m_objLogger, t_strLog);
        return -200;
    }
#endif MEMORYDATA
    return 0;
}

/// 处理所有插入或者更新cust_fund的操作,ret:0 正常
int axapi::TradeAPI::insertorUpdateFund(APINamespace CThostFtdcTradingAccountField *pTradingAccount)
{
    char* t_strLogFuncName = "TradeAPI::insertorUpdateFund";
    char t_strLog[2000];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    // sqlite版本
#ifdef SQLITE3DATA
    try {
        std::stringstream t_strQrySQL;
        t_strQrySQL << "select count(*) cnt from cust_fund where AccountID =";
        t_strQrySQL << " '" << pTradingAccount->AccountID << "';";
        std::string t_tmpstr = t_strQrySQL.str();
        const char* t_QrySQL = t_tmpstr.c_str();
        sprintf_s(t_strLog, 500, "%s:%s", t_strLogFuncName, t_QrySQL);
        LOG4CPLUS_DEBUG(m_objLogger, t_strLog);

        CppSQLite3Query q = m_objDB.execQuery(t_QrySQL);
        // 存在成交则更新资金信息,不存在则插入账户资金信息
        if (!q.eof())
        {
            if (*(q.fieldValue(0)) == '0')
            {
                // 不存在资金信息
                try
                {
                    // 构造insert sql语句
                    std::stringstream t_strIstSQL;
                    t_strIstSQL << "insert into cust_fund(";
                    t_strIstSQL << "BrokerID, AccountID, PreMortgage, PreCredit, PreDeposit, PreBalance, PreMargin";
                    t_strIstSQL << ", InterestBase, Interest, Deposit, Withdraw, FrozenMargin, FrozenCash, FrozenCommission";
                    t_strIstSQL << ", CurrMargin, CashIn, Commission, CloseProfit, PositionProfit, Balance, Available, WithdrawQuota";
                    t_strIstSQL << ", Reserve, TradingDay, SettlementID, Credit, Mortgage, ExchangeMargin, DeliveryMargin, ExchangeDeliveryMargin";
                    t_strIstSQL << ", ReserveBalance";
                    t_strIstSQL << ") values (";
                    t_strIstSQL << " '" << pTradingAccount->BrokerID << "'";
                    t_strIstSQL << ", '" << pTradingAccount->AccountID << "'";
                    t_strIstSQL << ", " << pTradingAccount->PreMortgage;
                    t_strIstSQL << ", " << pTradingAccount->PreCredit;
                    t_strIstSQL << ", " << pTradingAccount->PreDeposit;
                    t_strIstSQL << ", " << pTradingAccount->PreBalance;
                    t_strIstSQL << ", " << pTradingAccount->PreMargin;
                    t_strIstSQL << ", " << pTradingAccount->InterestBase;
                    t_strIstSQL << ", " << pTradingAccount->Interest;
                    t_strIstSQL << ", " << pTradingAccount->Deposit;
                    t_strIstSQL << ", " << pTradingAccount->Withdraw;
                    t_strIstSQL << ", " << pTradingAccount->FrozenMargin;
                    t_strIstSQL << ", " << pTradingAccount->FrozenCash;
                    t_strIstSQL << ", " << pTradingAccount->FrozenCommission;
                    t_strIstSQL << ", " << pTradingAccount->CurrMargin;
                    t_strIstSQL << ", " << pTradingAccount->CashIn;
                    t_strIstSQL << ", " << pTradingAccount->Commission;
                    t_strIstSQL << ", " << pTradingAccount->CloseProfit;
                    t_strIstSQL << ", " << pTradingAccount->PositionProfit;
                    t_strIstSQL << ", " << pTradingAccount->Balance;
                    t_strIstSQL << ", " << pTradingAccount->Available;
                    t_strIstSQL << ", " << pTradingAccount->WithdrawQuota;
                    t_strIstSQL << ", " << pTradingAccount->Reserve;
                    t_strIstSQL << ", '" << pTradingAccount->TradingDay << "'";
                    t_strIstSQL << ", " << pTradingAccount->SettlementID;
                    t_strIstSQL << ", " << pTradingAccount->Credit;
                    t_strIstSQL << ", " << pTradingAccount->Mortgage;
                    t_strIstSQL << ", " << pTradingAccount->ExchangeMargin;
                    t_strIstSQL << ", " << pTradingAccount->DeliveryMargin;
                    t_strIstSQL << ", " << pTradingAccount->ExchangeDeliveryMargin;
                    t_strIstSQL << ", " << pTradingAccount->ReserveBalance;
                    t_strIstSQL << " );";
                    t_tmpstr = t_strIstSQL.str();
                    const char* t_IstSQL = t_tmpstr.c_str();
                    sprintf_s(t_strLog, 2000, "%s:%s", t_strLogFuncName, t_IstSQL);
                    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
                    sprintf_s(t_strLog, 2000, "%s:insert %s|%d", t_strLogFuncName, pTradingAccount->AccountID, pTradingAccount->Available);
                    LOG4CPLUS_INFO(m_objLogger, t_strLog);

                    // 执行insert sql语句
                    int t_res = m_objDB.execDML(t_IstSQL);
                    // 判断处理结果
                    if (t_res != 1)
                    {
                        sprintf_s(t_strLog, 500, "%s:执行insert SQL 出错.", t_strLogFuncName);
                        LOG4CPLUS_ERROR(m_objLogger, t_strLog);
                        return -100;
                    }
                    sprintf_s(t_strLog, 500, "%s:执行insert SQL完成.", t_strLogFuncName);
                    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
                }
                catch (CppSQLite3Exception& e)
                {
                    sprintf_s(t_strLog, 500, "%s:插入cust_fund错误|%d|%s", t_strLogFuncName, e.errorCode(), e.errorMessage());
                    LOG4CPLUS_ERROR(m_objLogger, t_strLog);
                    return -100;
                }
            }
            else
            {
                // 存在资金信息
                try
                {
                    // 构造update sql语句
                    std::stringstream t_strUpdSQL;
                    t_strUpdSQL << "update cust_fund set";
                    t_strUpdSQL << " AccountID=" << "'" << pTradingAccount->AccountID << "'";
                    t_strUpdSQL << ", Deposit=" << pTradingAccount->Deposit;
                    t_strUpdSQL << ", Withdraw=" << pTradingAccount->Withdraw;
                    t_strUpdSQL << ", FrozenMargin=" << pTradingAccount->FrozenMargin;
                    t_strUpdSQL << ", FrozenCash=" << pTradingAccount->FrozenCash;
                    t_strUpdSQL << ", FrozenCommission=" << pTradingAccount->FrozenCommission;
                    t_strUpdSQL << ", CurrMargin=" << pTradingAccount->CurrMargin;
                    t_strUpdSQL << ", CashIn=" << pTradingAccount->CashIn;
                    t_strUpdSQL << ", Commission=" << pTradingAccount->Commission;
                    t_strUpdSQL << ", CloseProfit=" << pTradingAccount->CloseProfit;
                    t_strUpdSQL << ", PositionProfit=" << pTradingAccount->PositionProfit;
                    t_strUpdSQL << ", Balance=" << pTradingAccount->Balance;
                    t_strUpdSQL << ", Available=" << pTradingAccount->Available;
                    t_strUpdSQL << ", WithdrawQuota=" << pTradingAccount->WithdrawQuota;
                    t_strUpdSQL << ", Reserve=" << pTradingAccount->Reserve;
                    t_strUpdSQL << ", Credit=" << pTradingAccount->Credit;
                    t_strUpdSQL << ", Mortgage=" << pTradingAccount->Mortgage;
                    t_strUpdSQL << ", ExchangeMargin=" << pTradingAccount->ExchangeMargin;
                    t_strUpdSQL << ", DeliveryMargin=" << pTradingAccount->DeliveryMargin;
                    t_strUpdSQL << ", ExchangeDeliveryMargin=" << pTradingAccount->ExchangeDeliveryMargin;
                    t_strUpdSQL << ", ReserveBalance=" << pTradingAccount->ReserveBalance;
                    t_strUpdSQL << " where AccountID=" << "'" << pTradingAccount->AccountID << "'";
                    t_strUpdSQL << " ;";
                    t_tmpstr = t_strUpdSQL.str();
                    const char* t_UpdSQL = t_tmpstr.c_str();
                    sprintf_s(t_strLog, 2000, "%s:%s", t_strLogFuncName, t_UpdSQL);
                    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
                    sprintf_s(t_strLog, 2000, "%s:update %s|%d", t_strLogFuncName, pTradingAccount->AccountID, pTradingAccount->Available);
                    LOG4CPLUS_INFO(m_objLogger, t_strLog);

                    // 执行update sql语句
                    int t_res = m_objDB.execDML(t_UpdSQL);
                    // 判断处理结果
                    if (t_res != 1)
                    {
                        sprintf_s(t_strLog, 500, "%s:执行update SQL 出错.", t_strLogFuncName);
                        LOG4CPLUS_ERROR(m_objLogger, t_strLog);
                        return -100;
                    }
                    sprintf_s(t_strLog, 500, "%s:执行update SQL完成.", t_strLogFuncName);
                    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
                }
                catch (CppSQLite3Exception& e)
                {
                    sprintf_s(t_strLog, 500, "%s:更新cust_fund错误|%d|%s", t_strLogFuncName, e.errorCode(), e.errorMessage());
                    LOG4CPLUS_ERROR(m_objLogger, t_strLog);
                    return -100;
                }
            }
        }
        else
        {
            sprintf_s(t_strLog, 500, "%s:cust_fund查询失败|%s", t_strLogFuncName, pTradingAccount->AccountID);
            LOG4CPLUS_ERROR(m_objLogger, t_strLog);
            return -100;
        }
    }
    catch (CppSQLite3Exception& e)
    {
        sprintf_s(t_strLog, 500, "%s:CppSQLite3错误|%d|%s", t_strLogFuncName, e.errorCode(), e.errorMessage());
        LOG4CPLUS_FATAL(m_objLogger, t_strLog);
        return -200;
    }
#endif SQLITE3DATA
    // 内存版本
#ifdef MEMORYDATA
#endif MEMORYDATA
    return 0;
}

/// 处理所有插入或者更新cust_stock_hold_detail的操作,ret:0 正常
int axapi::TradeAPI::insertorUpdateSTKHoldDetail(APINamespace CThostFtdcInvestorPositionDetailField *pInvestorPositionDetail)
{
    char* t_strLogFuncName = "TradeAPI::insertorUpdateSTKHoldDetail";
    char t_strLog[2000];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    // sqlite版本
#ifdef SQLITE3DATA
    try {
        std::stringstream t_strQrySQL;
        t_strQrySQL << "select count(*) cnt from cust_stock_hold_detail where";
        t_strQrySQL << " TradingDay = '" << pInvestorPositionDetail->TradingDay << "'";
        t_strQrySQL << " and SettlementID = '" << pInvestorPositionDetail->SettlementID << "';";
        std::string t_tmpstr = t_strQrySQL.str();
        const char* t_QrySQL = t_tmpstr.c_str();
        sprintf_s(t_strLog, 500, "%s:%s", t_strLogFuncName, t_QrySQL);
        LOG4CPLUS_DEBUG(m_objLogger, t_strLog);

        CppSQLite3Query q = m_objDB.execQuery(t_QrySQL);
        // 存在持仓则更新持仓信息,不存在则插入账户持仓信息
        if (!q.eof())
        {
            if (*(q.fieldValue(0)) == '0')
            {
                // 不存在持仓信息
                try
                {
                    // 构造insert sql语句
                    std::stringstream t_strIstSQL;
                    t_strIstSQL << "insert into cust_stock_hold_detail(";
                    t_strIstSQL << "InstrumentID, BrokerID, InvestorID, HedgeFlag, Direction, OpenDate, TradeID, Volume, OpenPrice, TradingDay";
                    t_strIstSQL << ", SettlementID, TradeType, CombInstrumentID, ExchangeID, CloseProfitByDate, CloseProfitByTrade";
                    t_strIstSQL << ", PositionProfitByDate, PositionProfitByTrade, Margin, ExchMargin, MarginRateByMoney, MarginRateByVolume";
                    t_strIstSQL << ", LastSettlementPrice, SettlementPrice, CloseVolume, CloseAmount";
                    t_strIstSQL << ") values (";
                    t_strIstSQL << " '" << pInvestorPositionDetail->InstrumentID << "'";
                    t_strIstSQL << ", '" << pInvestorPositionDetail->BrokerID << "'";
                    t_strIstSQL << ", '" << pInvestorPositionDetail->InvestorID << "'";
                    t_strIstSQL << ", '" << pInvestorPositionDetail->HedgeFlag << "'";
                    t_strIstSQL << ", '" << pInvestorPositionDetail->Direction << "'";
                    t_strIstSQL << ", '" << pInvestorPositionDetail->OpenDate << "'";
                    t_strIstSQL << ", '" << pInvestorPositionDetail->TradeID << "'";
                    t_strIstSQL << ", " << pInvestorPositionDetail->Volume;
                    t_strIstSQL << ", " << pInvestorPositionDetail->OpenPrice;
                    t_strIstSQL << ", '" << pInvestorPositionDetail->TradingDay << "'";
                    t_strIstSQL << ", " << pInvestorPositionDetail->SettlementID;
                    t_strIstSQL << ", '" << pInvestorPositionDetail->TradeType << "'";
                    t_strIstSQL << ", '" << pInvestorPositionDetail->CombInstrumentID << "'";
                    t_strIstSQL << ", '" << pInvestorPositionDetail->ExchangeID << "'";
                    t_strIstSQL << ", " << pInvestorPositionDetail->CloseProfitByDate;
                    t_strIstSQL << ", " << pInvestorPositionDetail->CloseProfitByTrade;
                    t_strIstSQL << ", " << pInvestorPositionDetail->PositionProfitByDate;
                    t_strIstSQL << ", " << pInvestorPositionDetail->PositionProfitByTrade;
                    t_strIstSQL << ", " << pInvestorPositionDetail->Margin;
                    t_strIstSQL << ", " << pInvestorPositionDetail->ExchMargin;
                    t_strIstSQL << ", " << pInvestorPositionDetail->MarginRateByMoney;
                    t_strIstSQL << ", " << pInvestorPositionDetail->MarginRateByVolume;
                    t_strIstSQL << ", " << pInvestorPositionDetail->LastSettlementPrice;
                    t_strIstSQL << ", " << pInvestorPositionDetail->SettlementPrice;
                    t_strIstSQL << ", " << pInvestorPositionDetail->CloseVolume;
                    t_strIstSQL << ", " << pInvestorPositionDetail->CloseAmount;
                    t_strIstSQL << " );";
                    t_tmpstr = t_strIstSQL.str();
                    const char* t_IstSQL = t_tmpstr.c_str();
                    sprintf_s(t_strLog, 2000, "%s:%s", t_strLogFuncName, t_IstSQL);
                    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
                    sprintf_s(t_strLog, 2000, "%s:insert %s|%c|%d", t_strLogFuncName, pInvestorPositionDetail->InvestorID, pInvestorPositionDetail->Direction, pInvestorPositionDetail->Volume);
                    LOG4CPLUS_INFO(m_objLogger, t_strLog);

                    // 执行insert sql语句
                    int t_res = m_objDB.execDML(t_IstSQL);
                    // 判断处理结果
                    if (t_res != 1)
                    {
                        sprintf_s(t_strLog, 500, "%s:执行insert SQL 出错.", t_strLogFuncName);
                        LOG4CPLUS_ERROR(m_objLogger, t_strLog);
                        return -100;
                    }
                    sprintf_s(t_strLog, 500, "%s:执行insert SQL完成.", t_strLogFuncName);
                    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
                }
                catch (CppSQLite3Exception& e)
                {
                    sprintf_s(t_strLog, 500, "%s:插入cust_stock_hold_detail错误|%d|%s", t_strLogFuncName, e.errorCode(), e.errorMessage());
                    LOG4CPLUS_ERROR(m_objLogger, t_strLog);
                    return -100;
                }
            }
            else
            {
                // 存在持仓信息
                try
                {
                    // 构造update sql语句
                    std::stringstream t_strUpdSQL;
                    t_strUpdSQL << "update cust_stock_hold_detail set";
                    t_strUpdSQL << " InstrumentID=" << "'" << pInvestorPositionDetail->InstrumentID << "'";
                    t_strUpdSQL << ",BrokerID=" << "'" << pInvestorPositionDetail->BrokerID << "'";
                    t_strUpdSQL << ",InvestorID=" << "'" << pInvestorPositionDetail->InvestorID << "'";
                    t_strUpdSQL << ",HedgeFlag=" << "'" << pInvestorPositionDetail->HedgeFlag << "'";
                    t_strUpdSQL << ",Direction=" << "'" << pInvestorPositionDetail->Direction << "'";
                    t_strUpdSQL << ",OpenDate=" << "'" << pInvestorPositionDetail->OpenDate << "'";
                    t_strUpdSQL << ",TradeID=" << "'" << pInvestorPositionDetail->TradeID << "'";
                    t_strUpdSQL << ",Volume=" << pInvestorPositionDetail->Volume;
                    t_strUpdSQL << ",OpenPrice=" << pInvestorPositionDetail->OpenPrice;
                    t_strUpdSQL << ",TradingDay=" << "'" << pInvestorPositionDetail->TradingDay << "'";
                    t_strUpdSQL << ",SettlementID=" << pInvestorPositionDetail->SettlementID;
                    t_strUpdSQL << ",TradeType=" << pInvestorPositionDetail->TradeType;
                    t_strUpdSQL << ",CombInstrumentID=" << "'" << pInvestorPositionDetail->CombInstrumentID << "'";
                    t_strUpdSQL << ",ExchangeID=" << "'" << pInvestorPositionDetail->ExchangeID << "'";
                    t_strUpdSQL << ",CloseProfitByDate=" << pInvestorPositionDetail->CloseProfitByDate;
                    t_strUpdSQL << ",CloseProfitByTrade=" << pInvestorPositionDetail->CloseProfitByTrade;
                    t_strUpdSQL << ",PositionProfitByDate=" << pInvestorPositionDetail->PositionProfitByDate;
                    t_strUpdSQL << ",PositionProfitByTrade=" << pInvestorPositionDetail->PositionProfitByTrade;
                    t_strUpdSQL << ",Margin=" << pInvestorPositionDetail->Margin;
                    t_strUpdSQL << ",ExchMargin=" << pInvestorPositionDetail->ExchMargin;
                    t_strUpdSQL << ",MarginRateByMoney=" << pInvestorPositionDetail->MarginRateByMoney;
                    t_strUpdSQL << ",MarginRateByVolume=" << pInvestorPositionDetail->MarginRateByVolume;
                    t_strUpdSQL << ",LastSettlementPrice=" << pInvestorPositionDetail->LastSettlementPrice;
                    t_strUpdSQL << ",SettlementPrice=" << pInvestorPositionDetail->SettlementPrice;
                    t_strUpdSQL << ",CloseVolume=" << pInvestorPositionDetail->CloseVolume;
                    t_strUpdSQL << ",CloseAmount=" << pInvestorPositionDetail->CloseAmount;
                    t_strUpdSQL << " where TradingDay = '" << pInvestorPositionDetail->TradingDay << "'";
                    t_strUpdSQL << " and SettlementID = '" << pInvestorPositionDetail->SettlementID << "'";
                    t_strUpdSQL << " ;";
                    t_tmpstr = t_strUpdSQL.str();
                    const char* t_UpdSQL = t_tmpstr.c_str();
                    sprintf_s(t_strLog, 2000, "%s:%s", t_strLogFuncName, t_UpdSQL);
                    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
                    sprintf_s(t_strLog, 2000, "%s:update %s|%c|%d", t_strLogFuncName, pInvestorPositionDetail->InstrumentID, pInvestorPositionDetail->Direction, pInvestorPositionDetail->Volume);
                    LOG4CPLUS_INFO(m_objLogger, t_strLog);

                    // 执行update sql语句
                    int t_res = m_objDB.execDML(t_UpdSQL);
                    // 判断处理结果
                    if (t_res != 1)
                    {
                        sprintf_s(t_strLog, 500, "%s:执行update SQL 出错.", t_strLogFuncName);
                        LOG4CPLUS_ERROR(m_objLogger, t_strLog);
                        return -100;
                    }
                    sprintf_s(t_strLog, 500, "%s:执行update SQL完成.", t_strLogFuncName);
                    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
                }
                catch (CppSQLite3Exception& e)
                {
                    sprintf_s(t_strLog, 500, "%s:更新cust_stock_hold_detail错误|%d|%s", t_strLogFuncName, e.errorCode(), e.errorMessage());
                    LOG4CPLUS_ERROR(m_objLogger, t_strLog);
                    return -100;
                }
            }
        }
        else
        {
            sprintf_s(t_strLog, 500, "%s:cust_stock_hold_detail查询失败|%s", t_strLogFuncName, pInvestorPositionDetail->InvestorID);
            LOG4CPLUS_ERROR(m_objLogger, t_strLog);
            return -100;
        }
    }
    catch (CppSQLite3Exception& e)
    {
        sprintf_s(t_strLog, 500, "%s:CppSQLite3错误|%d|%s", t_strLogFuncName, e.errorCode(), e.errorMessage());
        LOG4CPLUS_FATAL(m_objLogger, t_strLog);
        return -200;
    }
#endif SQLITE3DATA
    // 内存版本
#ifdef MEMORYDATA
    try
    {
        int t_nUpdatePosition = -1;
        for (unsigned int i = 0; i < m_vInvestorPositionDetailList.size(); i++)
        {
            if (strcmp(m_vInvestorPositionDetailList[i].TradeID, pInvestorPositionDetail->TradeID) == 0)
            {
                t_nUpdatePosition = i;
                sprintf_s(t_strLog, 500, "%s:find recorded position detail info m_vInvestorPositionDetailList[%d]:%s", t_strLogFuncName, t_nUpdatePosition, m_vInvestorPositionDetailList[t_nUpdatePosition].TradeID);
                LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
                break;
            }
        }
        if (t_nUpdatePosition == -1)
        {
            APINamespace CThostFtdcInvestorPositionDetailField t_positionDetailfield;
            memset(&t_positionDetailfield, '\0', sizeof(t_positionDetailfield));
            memcpy_s(&t_positionDetailfield, sizeof(t_positionDetailfield), pInvestorPositionDetail, sizeof(*pInvestorPositionDetail));
            m_vInvestorPositionDetailList.push_back(t_positionDetailfield);
            sprintf_s(t_strLog, 500, "%s:insert data(%s) to m_vInvestorPositionDetailList[%d]", t_strLogFuncName, m_vInvestorPositionDetailList[m_vInvestorPositionDetailList.size() - 1].TradeID, m_vInvestorPositionDetailList.size() - 1);
            LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
        }
        else
        {
            m_vInvestorPositionDetailList[t_nUpdatePosition].Volume = pInvestorPositionDetail->Volume;
            m_vInvestorPositionDetailList[t_nUpdatePosition].OpenPrice = pInvestorPositionDetail->OpenPrice;
            sprintf_s(t_strLog, 500, "%s:update data(%s) in m_vInvestorPositionDetailList[%d]", t_strLogFuncName, m_vInvestorPositionDetailList[t_nUpdatePosition].TradeID, t_nUpdatePosition);
            LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
        }
    }
    catch (std::exception e)
    {
        sprintf_s(t_strLog, 500, "%s:错误%s", t_strLogFuncName, e.what());
        LOG4CPLUS_ERROR(m_objLogger, t_strLog);
        return -200;
    }
#endif MEMORYDATA
    return 0;
}

/// 处理所有插入或者更新cust_stock_hold_detail的操作,ret:0 正常
int axapi::TradeAPI::insertorUpdateSTKHold(APINamespace CThostFtdcInvestorPositionField *pInvestorPosition)
{
    char* t_strLogFuncName = "TradeAPI::insertorUpdateSTKHold";
    char t_strLog[2000];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    // sqlite版本
#ifdef SQLITE3DATA
    try {
        std::stringstream t_strQrySQL;
        t_strQrySQL << "select count(*) cnt from cust_stock_hold where";
        t_strQrySQL << " InvestorID = '" << pInvestorPosition->InvestorID << "'";
        t_strQrySQL << " and InstrumentID = '" << pInvestorPosition->InstrumentID << "'";
        t_strQrySQL << " and PosiDirection = '" << pInvestorPosition->PosiDirection << "'";
        t_strQrySQL << " and HedgeFlag = '" << pInvestorPosition->HedgeFlag << "';";
        std::string t_tmpstr = t_strQrySQL.str();
        const char* t_QrySQL = t_tmpstr.c_str();
        sprintf_s(t_strLog, 500, "%s:%s", t_strLogFuncName, t_QrySQL);
        LOG4CPLUS_DEBUG(m_objLogger, t_strLog);

        CppSQLite3Query q = m_objDB.execQuery(t_QrySQL);
        // 存在持仓则更新持仓信息,不存在则插入账户持仓信息
        if (!q.eof())
        {
            if (*(q.fieldValue(0)) == '0')
            {
                // 不存在持仓信息
                try
                {
                    // 构造insert sql语句
                    std::stringstream t_strIstSQL;
                    t_strIstSQL << "insert into cust_stock_hold(";
                    t_strIstSQL << "InstrumentID,BrokerID,InvestorID,PosiDirection,HedgeFlag,PositionDate,YdPosition,Position";
                    t_strIstSQL << ",LongFrozen,ShortFrozen,LongFrozenAmount,ShortFrozenAmount,OpenVolume,CloseVolume,OpenAmount";
                    t_strIstSQL << ",CloseAmount,PositionCost,PreMargin,UseMargin,FrozenMargin,FrozenCash,FrozenCommission,CashIn";
                    t_strIstSQL << ",Commission,CloseProfit,PositionProfit,PreSettlementPrice,SettlementPrice,TradingDay,SettlementID";
                    t_strIstSQL << ",OpenCost,ExchangeMargin,CombPosition,CombLongFrozen,CombShortFrozen,CloseProfitByDate,CloseProfitByTrade";
                    t_strIstSQL << ",TodayPosition,MarginRateByMoney,MarginRateByVolume";
                    t_strIstSQL << ") values (";
                    t_strIstSQL << "'" << pInvestorPosition->InstrumentID << "'";
                    t_strIstSQL << ",'" << pInvestorPosition->BrokerID << "'";
                    t_strIstSQL << ",'" << pInvestorPosition->InvestorID << "'";
                    t_strIstSQL << ",'" << pInvestorPosition->PosiDirection << "'";
                    t_strIstSQL << ",'" << pInvestorPosition->HedgeFlag << "'";
                    t_strIstSQL << ",'" << pInvestorPosition->PositionDate << "'";
                    t_strIstSQL << "," << pInvestorPosition->YdPosition;
                    t_strIstSQL << "," << pInvestorPosition->Position;
                    t_strIstSQL << "," << pInvestorPosition->LongFrozen;
                    t_strIstSQL << "," << pInvestorPosition->ShortFrozen;
                    t_strIstSQL << "," << pInvestorPosition->LongFrozenAmount;
                    t_strIstSQL << "," << pInvestorPosition->ShortFrozenAmount;
                    t_strIstSQL << "," << pInvestorPosition->OpenVolume;
                    t_strIstSQL << "," << pInvestorPosition->CloseVolume;
                    t_strIstSQL << "," << pInvestorPosition->OpenAmount;
                    t_strIstSQL << "," << pInvestorPosition->CloseAmount;
                    t_strIstSQL << "," << pInvestorPosition->PositionCost;
                    t_strIstSQL << "," << pInvestorPosition->PreMargin;
                    t_strIstSQL << "," << pInvestorPosition->UseMargin;
                    t_strIstSQL << "," << pInvestorPosition->FrozenMargin;
                    t_strIstSQL << "," << pInvestorPosition->FrozenCash;
                    t_strIstSQL << "," << pInvestorPosition->FrozenCommission;
                    t_strIstSQL << "," << pInvestorPosition->CashIn;
                    t_strIstSQL << "," << pInvestorPosition->Commission;
                    t_strIstSQL << "," << pInvestorPosition->CloseProfit;
                    t_strIstSQL << "," << pInvestorPosition->PositionProfit;
                    t_strIstSQL << "," << pInvestorPosition->PreSettlementPrice;
                    t_strIstSQL << "," << pInvestorPosition->SettlementPrice;
                    t_strIstSQL << ",'" << pInvestorPosition->TradingDay << "'";
                    t_strIstSQL << "," << pInvestorPosition->SettlementID;
                    t_strIstSQL << "," << pInvestorPosition->OpenCost;
                    t_strIstSQL << "," << pInvestorPosition->ExchangeMargin;
                    t_strIstSQL << "," << pInvestorPosition->CombPosition;
                    t_strIstSQL << "," << pInvestorPosition->CombLongFrozen;
                    t_strIstSQL << "," << pInvestorPosition->CombShortFrozen;
                    t_strIstSQL << "," << pInvestorPosition->CloseProfitByDate;
                    t_strIstSQL << "," << pInvestorPosition->CloseProfitByTrade;
                    t_strIstSQL << "," << pInvestorPosition->TodayPosition;
                    t_strIstSQL << "," << pInvestorPosition->MarginRateByMoney;
                    t_strIstSQL << "," << pInvestorPosition->MarginRateByVolume;
                    t_strIstSQL << " );";
                    t_tmpstr = t_strIstSQL.str();
                    const char* t_IstSQL = t_tmpstr.c_str();
                    sprintf_s(t_strLog, 2000, "%s:%s", t_strLogFuncName, t_IstSQL);
                    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
                    sprintf_s(t_strLog, 2000, "%s:insert %s|%c|%d", t_strLogFuncName, pInvestorPosition->InstrumentID, pInvestorPosition->PosiDirection, pInvestorPosition->OpenVolume);
                    LOG4CPLUS_INFO(m_objLogger, t_strLog);

                    // 执行insert sql语句
                    int t_res = m_objDB.execDML(t_IstSQL);
                    // 判断处理结果
                    if (t_res != 1)
                    {
                        sprintf_s(t_strLog, 500, "%s:执行insert SQL 出错.", t_strLogFuncName);
                        LOG4CPLUS_ERROR(m_objLogger, t_strLog);
                        return -100;
                    }
                    sprintf_s(t_strLog, 500, "%s:执行insert SQL完成.", t_strLogFuncName);
                    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
                }
                catch (CppSQLite3Exception& e)
                {
                    sprintf_s(t_strLog, 500, "%s:插入cust_stock_hold错误|%d|%s", t_strLogFuncName, e.errorCode(), e.errorMessage());
                    LOG4CPLUS_ERROR(m_objLogger, t_strLog);
                    return -100;
                }
            }
            else
            {
                // 存在持仓信息
                try
                {
                    // 构造update sql语句
                    std::stringstream t_strUpdSQL;
                    t_strUpdSQL << "update cust_stock_hold set";
                    t_strUpdSQL << " InstrumentID=" << "'" << pInvestorPosition->InstrumentID << "'";
                    t_strUpdSQL << ",BrokerID=" << "'" << pInvestorPosition->BrokerID << "'";
                    t_strUpdSQL << ",InvestorID=" << "'" << pInvestorPosition->InvestorID << "'";
                    t_strUpdSQL << ",PosiDirection=" << "'" << pInvestorPosition->PosiDirection << "'";
                    t_strUpdSQL << ",HedgeFlag=" << "'" << pInvestorPosition->HedgeFlag << "'";
                    t_strUpdSQL << ",PositionDate=" << "'" << pInvestorPosition->PositionDate << "'";
                    t_strUpdSQL << ",YdPosition=" << pInvestorPosition->YdPosition;
                    t_strUpdSQL << ",Position=" << pInvestorPosition->Position;
                    t_strUpdSQL << ",LongFrozen=" << pInvestorPosition->LongFrozen;
                    t_strUpdSQL << ",ShortFrozen=" << pInvestorPosition->ShortFrozen;
                    t_strUpdSQL << ",LongFrozenAmount=" << pInvestorPosition->LongFrozenAmount;
                    t_strUpdSQL << ",ShortFrozenAmount=" << pInvestorPosition->ShortFrozenAmount;
                    t_strUpdSQL << ",OpenVolume=" << pInvestorPosition->OpenVolume;
                    t_strUpdSQL << ",CloseVolume=" << pInvestorPosition->CloseVolume;
                    t_strUpdSQL << ",OpenAmount=" << pInvestorPosition->OpenAmount;
                    t_strUpdSQL << ",CloseAmount=" << pInvestorPosition->CloseAmount;
                    t_strUpdSQL << ",PositionCost=" << pInvestorPosition->PositionCost;
                    t_strUpdSQL << ",PreMargin=" << pInvestorPosition->PreMargin;
                    t_strUpdSQL << ",UseMargin=" << pInvestorPosition->UseMargin;
                    t_strUpdSQL << ",FrozenMargin=" << pInvestorPosition->FrozenMargin;
                    t_strUpdSQL << ",FrozenCash=" << pInvestorPosition->FrozenCash;
                    t_strUpdSQL << ",FrozenCommission=" << pInvestorPosition->FrozenCommission;
                    t_strUpdSQL << ",CashIn=" << pInvestorPosition->CashIn;
                    t_strUpdSQL << ",Commission=" << pInvestorPosition->Commission;
                    t_strUpdSQL << ",CloseProfit=" << pInvestorPosition->CloseProfit;
                    t_strUpdSQL << ",PositionProfit=" << pInvestorPosition->PositionProfit;
                    t_strUpdSQL << ",PreSettlementPrice=" << pInvestorPosition->PreSettlementPrice;
                    t_strUpdSQL << ",SettlementPrice=" << pInvestorPosition->SettlementPrice;
                    t_strUpdSQL << ",TradingDay=" << "'" << pInvestorPosition->TradingDay << "'";
                    t_strUpdSQL << ",SettlementID=" << pInvestorPosition->SettlementID;
                    t_strUpdSQL << ",OpenCost=" << pInvestorPosition->OpenCost;
                    t_strUpdSQL << ",ExchangeMargin=" << pInvestorPosition->ExchangeMargin;
                    t_strUpdSQL << ",CombPosition=" << pInvestorPosition->CombPosition;
                    t_strUpdSQL << ",CombLongFrozen=" << pInvestorPosition->CombLongFrozen;
                    t_strUpdSQL << ",CombShortFrozen=" << pInvestorPosition->CombShortFrozen;
                    t_strUpdSQL << ",CloseProfitByDate=" << pInvestorPosition->CloseProfitByDate;
                    t_strUpdSQL << ",CloseProfitByTrade=" << pInvestorPosition->CloseProfitByTrade;
                    t_strUpdSQL << ",TodayPosition=" << pInvestorPosition->TodayPosition;
                    t_strUpdSQL << ",MarginRateByMoney=" << pInvestorPosition->MarginRateByMoney;
                    t_strUpdSQL << ",MarginRateByVolume=" << pInvestorPosition->MarginRateByVolume;
                    t_strUpdSQL << " where InvestorID=" << "'" << pInvestorPosition->InvestorID << "'";
                    t_strUpdSQL << " and InstrumentID = '" << pInvestorPosition->InstrumentID << "'";
                    t_strUpdSQL << " and PosiDirection = '" << pInvestorPosition->PosiDirection << "'";
                    t_strUpdSQL << " and HedgeFlag = '" << pInvestorPosition->HedgeFlag << "'";
                    t_strUpdSQL << " ;";
                    t_tmpstr = t_strUpdSQL.str();
                    const char* t_UpdSQL = t_tmpstr.c_str();
                    sprintf_s(t_strLog, 2000, "%s:%s", t_strLogFuncName, t_UpdSQL);
                    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
                    sprintf_s(t_strLog, 2000, "%s:update %s|%c|%d", t_strLogFuncName, pInvestorPosition->InstrumentID, pInvestorPosition->PosiDirection, pInvestorPosition->OpenVolume);
                    LOG4CPLUS_INFO(m_objLogger, t_strLog);

                    // 执行update sql语句
                    int t_res = m_objDB.execDML(t_UpdSQL);
                    // 判断处理结果
                    if (t_res != 1)
                    {
                        sprintf_s(t_strLog, 500, "%s:执行update SQL 出错.", t_strLogFuncName);
                        LOG4CPLUS_ERROR(m_objLogger, t_strLog);
                        return -100;
                    }
                    sprintf_s(t_strLog, 500, "%s:执行update SQL完成.", t_strLogFuncName);
                    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
                }
                catch (CppSQLite3Exception& e)
                {
                    sprintf_s(t_strLog, 500, "%s:更新cust_stock_hold错误|%d|%s", t_strLogFuncName, e.errorCode(), e.errorMessage());
                    LOG4CPLUS_ERROR(m_objLogger, t_strLog);
                    return -100;
                }
            }
        }
        else
        {
            sprintf_s(t_strLog, 500, "%s:cust_stock_hold查询失败|%s", t_strLogFuncName, pInvestorPosition->InvestorID);
            LOG4CPLUS_ERROR(m_objLogger, t_strLog);
            return -100;
        }
    }
    catch (CppSQLite3Exception& e)
    {
        sprintf_s(t_strLog, 500, "%s:CppSQLite3错误|%d|%s", t_strLogFuncName, e.errorCode(), e.errorMessage());
        LOG4CPLUS_FATAL(m_objLogger, t_strLog);
        return -200;
    }
#endif SQLITE3DATA
    // 内存版本
#ifdef MEMORYDATA
    try
    {
        APINamespace CThostFtdcInvestorPositionField t_objInvestorPosition;
        memset(&t_objInvestorPosition, 0, sizeof(t_objInvestorPosition));
        strcpy_s(t_objInvestorPosition.BrokerID, sizeof(t_objInvestorPosition.BrokerID), pInvestorPosition->BrokerID);
        strcpy_s(t_objInvestorPosition.InstrumentID, sizeof(t_objInvestorPosition.InstrumentID), pInvestorPosition->InstrumentID);
        strcpy_s(t_objInvestorPosition.InvestorID, sizeof(t_objInvestorPosition.InvestorID), pInvestorPosition->InvestorID);
        t_objInvestorPosition.Position = pInvestorPosition->Position;
        t_objInvestorPosition.TodayPosition = pInvestorPosition->TodayPosition;
        t_objInvestorPosition.PosiDirection = pInvestorPosition->PosiDirection;
        m_vInvestorPositionList.push_back(t_objInvestorPosition);
    }
    catch (std::exception e)
    {
        sprintf_s(t_strLog, 500, "%s:错误%s", t_strLogFuncName, e.what());
        LOG4CPLUS_ERROR(m_objLogger, t_strLog);
        return -200;
    }
#endif MEMORYDATA
    return 0;
}

/// 处理所有插入或者更新market_data的操作,ret:0 正常
int axapi::TradeAPI::insertorUpdateMarketData(APINamespace CThostFtdcDepthMarketDataField *pDepthMarketData)
{
    char* t_strLogFuncName = "TradeAPI::insertorUpdateMarketData";
    char t_strLog[2000];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    // sqlite版本
#ifdef SQLITE3DATA
    try {
        std::stringstream t_strQrySQL;
        t_strQrySQL << "select count(*) cnt from market_data where";
        t_strQrySQL << " InstrumentID = '" << pDepthMarketData->InstrumentID << "';";
        std::string t_tmpstr = t_strQrySQL.str();
        const char* t_QrySQL = t_tmpstr.c_str();
        sprintf_s(t_strLog, 500, "%s:%s", t_strLogFuncName, t_QrySQL);
        LOG4CPLUS_DEBUG(m_objLogger, t_strLog);

        CppSQLite3Query q = m_objDB.execQuery(t_QrySQL);
        //存在合约行情则更新合约行情信息,不存在则插入合约行情信息
        if (!q.eof())
        {
            if (*(q.fieldValue(0)) == '0')
            {
                // 不存在行情信息
                try
                {
                    // 构造insert sql语句
                    std::stringstream t_strIstSQL;
                    t_strIstSQL << "insert into market_data(";
                    t_strIstSQL << "TradingDay,InstrumentID,ExchangeID,ExchangeInstID,LastPrice,PreSettlementPrice,PreClosePrice";
                    t_strIstSQL << ",PreOpenInterest,OpenPrice,HighestPrice,LowestPrice,Volume,Turnover,OpenInterest,ClosePrice";
                    t_strIstSQL << ",SettlementPrice,UpperLimitPrice,LowerLimitPrice,PreDelta,CurrDelta,UpdateTime,UpdateMillisec";
                    t_strIstSQL << ",BidPrice1,BidVolume1,AskPrice1,AskVolume1,BidPrice2,BidVolume2,AskPrice2,AskVolume2";
                    t_strIstSQL << ",BidPrice3,BidVolume3,AskPrice3,AskVolume3,BidPrice4,BidVolume4,AskPrice4,AskVolume4";
                    t_strIstSQL << ",BidPrice5,BidVolume5,AskPrice5,AskVolume5,AveragePrice,ActionDay";
                    t_strIstSQL << ") values (";
                    t_strIstSQL << "'" << pDepthMarketData->TradingDay << "'";
                    t_strIstSQL << ",'" << pDepthMarketData->InstrumentID << "'";
                    t_strIstSQL << ",'" << pDepthMarketData->ExchangeID << "'";
                    t_strIstSQL << ",'" << pDepthMarketData->ExchangeInstID << "'";
                    t_strIstSQL << "," << pDepthMarketData->LastPrice;
                    t_strIstSQL << "," << pDepthMarketData->PreSettlementPrice;
                    t_strIstSQL << "," << pDepthMarketData->PreClosePrice;
                    t_strIstSQL << "," << pDepthMarketData->PreOpenInterest;
                    t_strIstSQL << "," << pDepthMarketData->OpenPrice;
                    t_strIstSQL << "," << pDepthMarketData->HighestPrice;
                    t_strIstSQL << "," << pDepthMarketData->LowestPrice;
                    t_strIstSQL << "," << pDepthMarketData->Volume;
                    t_strIstSQL << "," << pDepthMarketData->Turnover;
                    t_strIstSQL << "," << pDepthMarketData->OpenInterest;
                    t_strIstSQL << "," << pDepthMarketData->ClosePrice;
                    t_strIstSQL << "," << pDepthMarketData->SettlementPrice;
                    t_strIstSQL << "," << pDepthMarketData->UpperLimitPrice;
                    t_strIstSQL << "," << pDepthMarketData->LowerLimitPrice;
                    t_strIstSQL << "," << pDepthMarketData->PreDelta;
                    t_strIstSQL << "," << pDepthMarketData->CurrDelta;
                    t_strIstSQL << ",'" << pDepthMarketData->UpdateTime << "'";
                    t_strIstSQL << "," << pDepthMarketData->UpdateMillisec;
                    t_strIstSQL << "," << pDepthMarketData->BidPrice1;
                    t_strIstSQL << "," << pDepthMarketData->BidVolume1;
                    t_strIstSQL << "," << pDepthMarketData->AskPrice1;
                    t_strIstSQL << "," << pDepthMarketData->AskVolume1;
                    t_strIstSQL << "," << pDepthMarketData->BidPrice2;
                    t_strIstSQL << "," << pDepthMarketData->BidVolume2;
                    t_strIstSQL << "," << pDepthMarketData->AskPrice2;
                    t_strIstSQL << "," << pDepthMarketData->AskVolume2;
                    t_strIstSQL << "," << pDepthMarketData->BidPrice3;
                    t_strIstSQL << "," << pDepthMarketData->BidVolume3;
                    t_strIstSQL << "," << pDepthMarketData->AskPrice3;
                    t_strIstSQL << "," << pDepthMarketData->AskVolume3;
                    t_strIstSQL << "," << pDepthMarketData->BidPrice4;
                    t_strIstSQL << "," << pDepthMarketData->BidVolume4;
                    t_strIstSQL << "," << pDepthMarketData->AskPrice4;
                    t_strIstSQL << "," << pDepthMarketData->AskVolume4;
                    t_strIstSQL << "," << pDepthMarketData->BidPrice5;
                    t_strIstSQL << "," << pDepthMarketData->BidVolume5;
                    t_strIstSQL << "," << pDepthMarketData->AskPrice5;
                    t_strIstSQL << "," << pDepthMarketData->AskVolume5;
                    t_strIstSQL << "," << pDepthMarketData->AveragePrice;
                    t_strIstSQL << ",'" << pDepthMarketData->ActionDay << "'";
                    t_strIstSQL << ");";
                    t_tmpstr = t_strIstSQL.str();
                    const char* t_IstSQL = t_tmpstr.c_str();
                    sprintf_s(t_strLog, 2000, "%s:%s", t_strLogFuncName, t_IstSQL);
                    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
                    sprintf_s(t_strLog, 2000, "%s:insert %s", t_strLogFuncName, pDepthMarketData->InstrumentID);
                    LOG4CPLUS_INFO(m_objLogger, t_strLog);

                    // 执行insert sql语句
                    int t_res = m_objDB.execDML(t_IstSQL);
                    // 判断处理结果
                    if (t_res != 1)
                    {
                        sprintf_s(t_strLog, 500, "%s:执行insert SQL 出错.", t_strLogFuncName);
                        LOG4CPLUS_ERROR(m_objLogger, t_strLog);
                        return -100;
                    }
                    sprintf_s(t_strLog, 500, "%s:执行insert SQL完成.", t_strLogFuncName);
                    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
                }
                catch (CppSQLite3Exception& e)
                {
                    sprintf_s(t_strLog, 500, "%s:插入market_data错误|%d|%s", t_strLogFuncName, e.errorCode(), e.errorMessage());
                    LOG4CPLUS_ERROR(m_objLogger, t_strLog);
                    return -100;
                }
            }
            else
            {
                // 存在行情信息
                try
                {
                    // 构造update sql语句
                    std::stringstream t_strUpdSQL;
                    t_strUpdSQL << "update market_data set";
                    t_strUpdSQL << " TradingDay=" << "'" << pDepthMarketData->TradingDay << "'";
                    t_strUpdSQL << ",ExchangeID=" << "'" << pDepthMarketData->ExchangeID << "'";
                    t_strUpdSQL << ",ExchangeInstID=" << "'" << pDepthMarketData->ExchangeInstID << "'";
                    t_strUpdSQL << ",LastPrice=" << pDepthMarketData->LastPrice;
                    t_strUpdSQL << ",PreSettlementPrice=" << pDepthMarketData->PreSettlementPrice;
                    t_strUpdSQL << ",PreClosePrice=" << pDepthMarketData->PreClosePrice;
                    t_strUpdSQL << ",PreOpenInterest=" << pDepthMarketData->PreOpenInterest;
                    t_strUpdSQL << ",OpenPrice=" << pDepthMarketData->OpenPrice;
                    t_strUpdSQL << ",HighestPrice=" << pDepthMarketData->HighestPrice;
                    t_strUpdSQL << ",LowestPrice=" << pDepthMarketData->LowestPrice;
                    t_strUpdSQL << ",Volume=" << pDepthMarketData->Volume;
                    t_strUpdSQL << ",Turnover=" << pDepthMarketData->Turnover;
                    t_strUpdSQL << ",OpenInterest=" << pDepthMarketData->OpenInterest;
                    t_strUpdSQL << ",ClosePrice=" << pDepthMarketData->ClosePrice;
                    t_strUpdSQL << ",SettlementPrice=" << pDepthMarketData->SettlementPrice;
                    t_strUpdSQL << ",UpperLimitPrice=" << pDepthMarketData->UpperLimitPrice;
                    t_strUpdSQL << ",LowerLimitPrice=" << pDepthMarketData->LowerLimitPrice;
                    t_strUpdSQL << ",PreDelta=" << pDepthMarketData->PreDelta;
                    t_strUpdSQL << ",CurrDelta=" << pDepthMarketData->CurrDelta;
                    t_strUpdSQL << ",UpdateTime=" << "'" << pDepthMarketData->UpdateTime << "'";
                    t_strUpdSQL << ",UpdateMillisec=" << pDepthMarketData->UpdateMillisec;
                    t_strUpdSQL << ",BidPrice1=" << pDepthMarketData->BidPrice1;
                    t_strUpdSQL << ",BidVolume1=" << pDepthMarketData->BidVolume1;
                    t_strUpdSQL << ",AskPrice1=" << pDepthMarketData->AskPrice1;
                    t_strUpdSQL << ",AskVolume1=" << pDepthMarketData->AskVolume1;
                    t_strUpdSQL << ",BidPrice2=" << pDepthMarketData->BidPrice2;
                    t_strUpdSQL << ",BidVolume2=" << pDepthMarketData->BidVolume2;
                    t_strUpdSQL << ",AskPrice2=" << pDepthMarketData->AskPrice2;
                    t_strUpdSQL << ",AskVolume2=" << pDepthMarketData->AskVolume2;
                    t_strUpdSQL << ",BidPrice3=" << pDepthMarketData->BidPrice3;
                    t_strUpdSQL << ",BidVolume3=" << pDepthMarketData->BidVolume3;
                    t_strUpdSQL << ",AskPrice3=" << pDepthMarketData->AskPrice3;
                    t_strUpdSQL << ",AskVolume3=" << pDepthMarketData->AskVolume3;
                    t_strUpdSQL << ",BidPrice4=" << pDepthMarketData->BidPrice4;
                    t_strUpdSQL << ",BidVolume4=" << pDepthMarketData->BidVolume4;
                    t_strUpdSQL << ",AskPrice4=" << pDepthMarketData->AskPrice4;
                    t_strUpdSQL << ",AskVolume4=" << pDepthMarketData->AskVolume4;
                    t_strUpdSQL << ",BidPrice5=" << pDepthMarketData->BidPrice5;
                    t_strUpdSQL << ",BidVolume5=" << pDepthMarketData->BidVolume5;
                    t_strUpdSQL << ",AskPrice5=" << pDepthMarketData->AskPrice5;
                    t_strUpdSQL << ",AskVolume5=" << pDepthMarketData->AskVolume5;
                    t_strUpdSQL << ",AveragePrice=" << pDepthMarketData->AveragePrice;
                    t_strUpdSQL << ",ActionDay=" << "'" << pDepthMarketData->ActionDay << "'";
                    t_strUpdSQL << " where";
                    t_strUpdSQL << " InstrumentID='" << pDepthMarketData->InstrumentID << "'";
                    t_strUpdSQL << ";";
                    t_tmpstr = t_strUpdSQL.str();
                    const char* t_UpdSQL = t_tmpstr.c_str();
                    sprintf_s(t_strLog, 2000, "%s:%s", t_strLogFuncName, t_UpdSQL);
                    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
                    sprintf_s(t_strLog, 2000, "%s:update %s", t_strLogFuncName, pDepthMarketData->InstrumentID);
                    LOG4CPLUS_INFO(m_objLogger, t_strLog);

                    // 执行update sql语句
                    int t_res = m_objDB.execDML(t_UpdSQL);
                    // 判断处理结果
                    if (t_res != 1)
                    {
                        sprintf_s(t_strLog, 500, "%s:执行update SQL 出错.", t_strLogFuncName);
                        LOG4CPLUS_ERROR(m_objLogger, t_strLog);
                        return -100;
                    }
                    sprintf_s(t_strLog, 500, "%s:执行update SQL完成.", t_strLogFuncName);
                    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
                }
                catch (CppSQLite3Exception& e)
                {
                    sprintf_s(t_strLog, 500, "%s:更新market_data错误|%d|%s", t_strLogFuncName, e.errorCode(), e.errorMessage());
                    LOG4CPLUS_ERROR(m_objLogger, t_strLog);
                    return -100;
                }
            }
        }
        else
        {
            sprintf_s(t_strLog, 500, "%s:market_data查询失败|%s", t_strLogFuncName, pDepthMarketData->InstrumentID);
            LOG4CPLUS_ERROR(m_objLogger, t_strLog);
            return -100;
        }
    }
    catch (CppSQLite3Exception& e)
    {
        sprintf_s(t_strLog, 500, "%s:CppSQLite3错误|%d|%s", t_strLogFuncName, e.errorCode(), e.errorMessage());
        LOG4CPLUS_FATAL(m_objLogger, t_strLog);
        return -200;
    }
#endif SQLITE3DATA
    // 内存版本
#ifdef MEMORYDATA
#endif MEMORYDATA
    return 0;
}

/// 处理所有插入或者更新instrument的操作,ret:0 正常
int axapi::TradeAPI::insertorUpdateInstrument(APINamespace CThostFtdcInstrumentField *pInstrument)
{
    char* t_strLogFuncName = "TradeAPI::insertorUpdateInstrument";
    char t_strLog[2000];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    // sqlite版本
#ifdef SQLITE3DATA
    try {
        std::stringstream t_strQrySQL;
        t_strQrySQL << "select count(*) cnt from instrument where";
        t_strQrySQL << " InstrumentID = '" << pInstrument->InstrumentID << "';";
        std::string t_tmpstr = t_strQrySQL.str();
        const char* t_QrySQL = t_tmpstr.c_str();
        sprintf_s(t_strLog, 500, "%s:%s", t_strLogFuncName, t_QrySQL);
        LOG4CPLUS_DEBUG(m_objLogger, t_strLog);

        CppSQLite3Query q = m_objDB.execQuery(t_QrySQL);
        // 存在合约则更新合约信息,不存在则插入合约信息
        if (!q.eof())
        {
            if (*(q.fieldValue(0)) == '0')
            {
                // 不存在合约信息
                try
                {
                    // 构造insert sql语句
                    std::stringstream t_strIstSQL;
                    t_strIstSQL << "insert into instrument(";
                    t_strIstSQL << "InstrumentID,ExchangeID,InstrumentName,ExchangeInstID,ProductID,ProductClass,DeliveryYear,DeliveryMonth";
                    t_strIstSQL << ",MaxMarketOrderVolume,MinMarketOrderVolume,MaxLimitOrderVolume,MinLimitOrderVolume,VolumeMultiple";
                    t_strIstSQL << ",PriceTick,CreateDate,OpenDate,ExpireDate,StartDelivDate,EndDelivDate,InstLifePhase,IsTrading";
                    t_strIstSQL << ",PositionType,PositionDateType,LongMarginRatio,ShortMarginRatio";
                    t_strIstSQL << ") values (";
                    t_strIstSQL << "'" << pInstrument->InstrumentID << "'";
                    t_strIstSQL << ",'" << pInstrument->ExchangeID << "'";
                    t_strIstSQL << ",'" << pInstrument->InstrumentName << "'";
                    t_strIstSQL << ",'" << pInstrument->ExchangeInstID << "'";
                    t_strIstSQL << ",'" << pInstrument->ProductID << "'";
                    t_strIstSQL << ",'" << pInstrument->ProductClass << "'";
                    t_strIstSQL << "," << pInstrument->DeliveryYear;
                    t_strIstSQL << "," << pInstrument->DeliveryMonth;
                    t_strIstSQL << "," << pInstrument->MaxMarketOrderVolume;
                    t_strIstSQL << "," << pInstrument->MinMarketOrderVolume;
                    t_strIstSQL << "," << pInstrument->MaxLimitOrderVolume;
                    t_strIstSQL << "," << pInstrument->MinLimitOrderVolume;
                    t_strIstSQL << "," << pInstrument->VolumeMultiple;
                    t_strIstSQL << "," << pInstrument->PriceTick;
                    t_strIstSQL << ",'" << pInstrument->CreateDate << "'";
                    t_strIstSQL << ",'" << pInstrument->OpenDate << "'";
                    t_strIstSQL << ",'" << pInstrument->ExpireDate << "'";
                    t_strIstSQL << ",'" << pInstrument->StartDelivDate << "'";
                    t_strIstSQL << ",'" << pInstrument->EndDelivDate << "'";
                    t_strIstSQL << ",'" << pInstrument->InstLifePhase << "'";
                    t_strIstSQL << "," << pInstrument->IsTrading;
                    t_strIstSQL << ",'" << pInstrument->PositionType << "'";
                    t_strIstSQL << ",'" << pInstrument->PositionDateType << "'";
                    t_strIstSQL << "," << pInstrument->LongMarginRatio;
                    t_strIstSQL << "," << pInstrument->ShortMarginRatio;
                    //t_strIstSQL << ",'" << pInstrument->MaxMarginSideAlgorithm << "'";
                    t_strIstSQL << ");";
                    t_tmpstr = t_strIstSQL.str();
                    const char* t_IstSQL = t_tmpstr.c_str();
                    sprintf_s(t_strLog, 2000, "%s:%s", t_strLogFuncName, t_IstSQL);
                    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);

                    // 执行insert sql语句
                    int t_res = m_objDB.execDML(t_IstSQL);
                    // 判断处理结果
                    if (t_res != 1)
                    {
                        sprintf_s(t_strLog, 500, "%s:执行insert SQL 出错.", t_strLogFuncName);
                        LOG4CPLUS_ERROR(m_objLogger, t_strLog);
                        return -100;
                    }
                    sprintf_s(t_strLog, 500, "%s:执行insert SQL完成.", t_strLogFuncName);
                    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
                }
                catch (CppSQLite3Exception& e)
                {
                    sprintf_s(t_strLog, 500, "%s:插入instrument错误|%d|%s", t_strLogFuncName, e.errorCode(), e.errorMessage());
                    LOG4CPLUS_ERROR(m_objLogger, t_strLog);
                    return -100;
                }
            }
            else
            {
                // 存在合约信息
                try
                {
                    // 构造update sql语句
                    std::stringstream t_strUpdSQL;
                    t_strUpdSQL << "update market_data set";
                    t_strUpdSQL << " ExchangeID=" << "'" << pInstrument->ExchangeID << "'";
                    t_strUpdSQL << ",InstrumentName=" << "'" << pInstrument->InstrumentName << "'";
                    t_strUpdSQL << ",ExchangeInstID=" << "'" << pInstrument->ExchangeInstID << "'";
                    t_strUpdSQL << ",ProductID=" << "'" << pInstrument->ProductID << "'";
                    t_strUpdSQL << ",ProductClass=" << "'" << pInstrument->ProductClass << "'";
                    t_strUpdSQL << ",DeliveryYear=" << pInstrument->DeliveryYear;
                    t_strUpdSQL << ",DeliveryMonth=" << pInstrument->DeliveryMonth;
                    t_strUpdSQL << ",MaxMarketOrderVolume=" << pInstrument->MaxMarketOrderVolume;
                    t_strUpdSQL << ",MinMarketOrderVolume=" << pInstrument->MinMarketOrderVolume;
                    t_strUpdSQL << ",MaxLimitOrderVolume=" << pInstrument->MaxLimitOrderVolume;
                    t_strUpdSQL << ",MinLimitOrderVolume=" << pInstrument->MinLimitOrderVolume;
                    t_strUpdSQL << ",VolumeMultiple=" << pInstrument->VolumeMultiple;
                    t_strUpdSQL << ",PriceTick=" << pInstrument->PriceTick;
                    t_strUpdSQL << ",CreateDate=" << "'" << pInstrument->CreateDate << "'";
                    t_strUpdSQL << ",OpenDate=" << "'" << pInstrument->OpenDate << "'";
                    t_strUpdSQL << ",ExpireDate=" << "'" << pInstrument->ExpireDate << "'";
                    t_strUpdSQL << ",StartDelivDate=" << "'" << pInstrument->StartDelivDate << "'";
                    t_strUpdSQL << ",EndDelivDate=" << "'" << pInstrument->EndDelivDate << "'";
                    t_strUpdSQL << ",InstLifePhase=" << "'" << pInstrument->InstLifePhase << "'";
                    t_strUpdSQL << ",IsTrading=" << pInstrument->IsTrading;
                    t_strUpdSQL << ",PositionType=" << "'" << pInstrument->PositionType << "'";
                    t_strUpdSQL << ",PositionDateType=" << "'" << pInstrument->PositionDateType << "'";
                    t_strUpdSQL << ",LongMarginRatio=" << pInstrument->LongMarginRatio;
                    t_strUpdSQL << ",ShortMarginRatio=" << pInstrument->ShortMarginRatio;
                    t_strUpdSQL << ",MaxMarginSideAlgorithm=" << "'" << pInstrument->MaxMarginSideAlgorithm << "'";
                    t_strUpdSQL << " where";
                    t_strUpdSQL << " InstrumentID = '" << pInstrument->InstrumentID << "'";
                    t_strUpdSQL << ";";
                    t_tmpstr = t_strUpdSQL.str();
                    const char* t_UpdSQL = t_tmpstr.c_str();
                    sprintf_s(t_strLog, 2000, "%s:%s", t_strLogFuncName, t_UpdSQL);
                    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);

                    // 执行update sql语句
                    int t_res = m_objDB.execDML(t_UpdSQL);
                    // 判断处理结果
                    if (t_res != 1)
                    {
                        sprintf_s(t_strLog, 500, "%s:执行update SQL 出错.", t_strLogFuncName);
                        LOG4CPLUS_ERROR(m_objLogger, t_strLog);
                        return -100;
                    }
                    sprintf_s(t_strLog, 500, "%s:执行update SQL完成.", t_strLogFuncName);
                    LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
                }
                catch (CppSQLite3Exception& e)
                {
                    sprintf_s(t_strLog, 500, "%s:更新instrument错误|%d|%s", t_strLogFuncName, e.errorCode(), e.errorMessage());
                    LOG4CPLUS_ERROR(m_objLogger, t_strLog);
                    return -100;
                }
            }
        }
        else
        {
            sprintf_s(t_strLog, 500, "%s:instrument查询失败|%s", t_strLogFuncName, pInstrument->InstrumentID);
            LOG4CPLUS_ERROR(m_objLogger, t_strLog);
            return -100;
        }
    }
    catch (CppSQLite3Exception& e)
    {
        sprintf_s(t_strLog, 500, "%s:CppSQLite3错误|%d|%s", t_strLogFuncName, e.errorCode(), e.errorMessage());
        LOG4CPLUS_FATAL(m_objLogger, t_strLog);
        return -200;
    }
#endif SQLITE3DATA
    // 内存版本
#ifdef MEMORYDATA
    try
    {
        m_vInstrumentList.push_back(*pInstrument);
    }
    catch (std::exception e)
    {
        return -200;
    }
#endif MEMORYDATA
    return 0;
}

/// 处理所有插入或者更新instrumentStatus的操作,ret:0 正常
int axapi::TradeAPI::insertorUpdateInstrumentStatus(APINamespace CThostFtdcInstrumentStatusField *pInstrumentStatus)
{
    char* t_strLogFuncName = "TradeAPI::insertorUpdateInstrument";
    char t_strLog[2000];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    // sqlite版本
#ifdef SQLITE3DATA
#endif SQLITE3DATA
    // 内存版本
#ifdef MEMORYDATA
    try
    {
        m_vInstrumentStatusList.push_back(*pInstrumentStatus);
    }
    catch (std::exception e)
    {
        return -200;
    }
#endif MEMORYDATA
    return 0;
}

/// 查询委托,结果入库或者进入内存结果,ret:Requestid
int axapi::TradeAPI::queryCustOrder()
{
    char* t_strLogFuncName = "TradeAPI::queryCustOrder";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    // 清除原有委托信息
#ifdef SQLITE3DATA
    clearDB("cust_order");
#endif SQLITE3DATA

    APINamespace CThostFtdcQryOrderField t_qryorder;
    memset(&t_qryorder, 0, sizeof(t_qryorder));
    strcpy_s(t_qryorder.BrokerID, sizeof(t_qryorder.BrokerID), m_strBrokerID);
    strcpy_s(t_qryorder.InvestorID, sizeof(t_qryorder.InvestorID), m_strUserID);
    m_pUserApi->ReqQryOrder(&t_qryorder, m_nRequestID++);

    sprintf_s(t_strLog, 500, "%s:查询委托请求ID-%d", t_strLogFuncName, m_nRequestID - 1);
    LOG4CPLUS_INFO(m_objLogger, t_strLog);

    return m_nRequestID - 1;
}

/// 查询成交,结果入库或者进入内存结果,ret:Requestid
int axapi::TradeAPI::queryCustDone()
{
    char* t_strLogFuncName = "TradeAPI::queryCustDone";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    // 清除原有成交信息
#ifdef SQLITE3DATA
    clearDB("cust_done");
#endif SQLITE3DATA

    APINamespace CThostFtdcQryTradeField t_qryTrade;
    memset(&t_qryTrade, 0, sizeof(t_qryTrade));
    strcpy_s(t_qryTrade.BrokerID, sizeof(t_qryTrade.BrokerID), m_strBrokerID);
    strcpy_s(t_qryTrade.InvestorID, sizeof(t_qryTrade.InvestorID), m_strUserID);
    m_pUserApi->ReqQryTrade(&t_qryTrade, m_nRequestID++);

    sprintf_s(t_strLog, 500, "%s:查询成交请求ID-%d", t_strLogFuncName, m_nRequestID - 1);
    LOG4CPLUS_INFO(m_objLogger, t_strLog);

    return m_nRequestID - 1;
}

/// 查询资金,结果入库或者进入内存结果,ret:Requestid
int axapi::TradeAPI::queryCustFund()
{
    char* t_strLogFuncName = "TradeAPI::queryCustDone";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    // 清除原有资金信息
#ifdef SQLITE3DATA
    clearDB("cust_fund");
#endif SQLITE3DATA

    APINamespace CThostFtdcQryTradingAccountField t_qryTradingAccount;
    memset(&t_qryTradingAccount, 0, sizeof(t_qryTradingAccount));
    strcpy_s(t_qryTradingAccount.BrokerID, sizeof(t_qryTradingAccount.BrokerID), m_strBrokerID);
    strcpy_s(t_qryTradingAccount.InvestorID, sizeof(t_qryTradingAccount.InvestorID), m_strUserID);
    m_pUserApi->ReqQryTradingAccount(&t_qryTradingAccount, m_nRequestID++);

    sprintf_s(t_strLog, 500, "%s:查询资金请求ID-%d", t_strLogFuncName, m_nRequestID - 1);
    LOG4CPLUS_INFO(m_objLogger, t_strLog);

    return m_nRequestID - 1;
}

/// 查询持仓,结果入库或者进入内存结果,ret:Requestid
int axapi::TradeAPI::queryCustSTKHoldDetail()
{
    char* t_strLogFuncName = "TradeAPI::queryCustSTKHoldDetail";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    // 清除原有持仓明细信息
#ifdef SQLITE3DATA
    clearDB("cust_stock_hold_detail");
#endif SQLITE3DATA

    m_vInvestorPositionDetailList.clear();

    APINamespace CThostFtdcQryInvestorPositionDetailField t_qryInvestorPositionDetail;
    memset(&t_qryInvestorPositionDetail, 0, sizeof(t_qryInvestorPositionDetail));
    strcpy_s(t_qryInvestorPositionDetail.BrokerID, sizeof(t_qryInvestorPositionDetail.BrokerID), m_strBrokerID);
    strcpy_s(t_qryInvestorPositionDetail.InvestorID, sizeof(t_qryInvestorPositionDetail.InvestorID), m_strUserID);
    m_pUserApi->ReqQryInvestorPositionDetail(&t_qryInvestorPositionDetail, m_nRequestID++);

    sprintf_s(t_strLog, 500, "%s:查询持仓明细请求ID-%d", t_strLogFuncName, m_nRequestID - 1);
    LOG4CPLUS_INFO(m_objLogger, t_strLog);

    return m_nRequestID - 1;
}

/// 查询持仓明细,结果入库或者进入内存结果,ret:Requestid
int axapi::TradeAPI::queryCustSTKHold()
{
    char* t_strLogFuncName = "TradeAPI::queryCustSTKHold";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    // 清除原有持仓信息
#ifdef SQLITE3DATA
    clearDB("cust_stock_hold");
#endif SQLITE3DATA

    m_vInvestorPositionList.clear();

    APINamespace CThostFtdcQryInvestorPositionField t_pQryInvestorPosition;
    memset(&t_pQryInvestorPosition, 0, sizeof(t_pQryInvestorPosition));
    strcpy_s(t_pQryInvestorPosition.BrokerID, sizeof(t_pQryInvestorPosition.BrokerID), m_strBrokerID);
    strcpy_s(t_pQryInvestorPosition.InvestorID, sizeof(t_pQryInvestorPosition.InvestorID), m_strUserID);
    m_pUserApi->ReqQryInvestorPosition(&t_pQryInvestorPosition, m_nRequestID++);

    sprintf_s(t_strLog, 500, "%s:查询持仓请求ID-%d", t_strLogFuncName, m_nRequestID - 1);
    LOG4CPLUS_INFO(m_objLogger, t_strLog);

    return m_nRequestID - 1;
}

/// 查询指定合约的行情,结果进入内存结果,可通过MarketQuotationAPI替换该功能 
int axapi::TradeAPI::queryMarketData(char* in_strContract)
{
    char* t_strLogFuncName = "TradeAPI::queryMarketData";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    APINamespace CThostFtdcQryDepthMarketDataField t_QryDepthMarketDataField;
    memset(&t_QryDepthMarketDataField, 0, sizeof(t_QryDepthMarketDataField));
    strcpy_s(t_QryDepthMarketDataField.InstrumentID, sizeof(t_QryDepthMarketDataField.InstrumentID), in_strContract);
    m_pUserApi->ReqQryDepthMarketData(&t_QryDepthMarketDataField, m_nRequestID++);

    sprintf_s(t_strLog, 500, "%s:查询行情请求ID-%d", t_strLogFuncName, m_nRequestID - 1);
    LOG4CPLUS_INFO(m_objLogger, t_strLog);

    return m_nRequestID - 1;
}

/// 查询合约信息,结果入库或者进入内存结果,ret:Requestid
int axapi::TradeAPI::queryInstrument()
{
    char* t_strLogFuncName = "TradeAPI::queryInstrument";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    // 清除原有合约信息
#ifdef SQLITE3DATA
    clearDB("instrument");
#endif SQLITE3DATA

    APINamespace CThostFtdcQryInstrumentField t_QryInstrumentField;
    memset(&t_QryInstrumentField, 0, sizeof(t_QryInstrumentField));
    m_pUserApi->ReqQryInstrument(&t_QryInstrumentField, m_nRequestID++);

    sprintf_s(t_strLog, 500, "%s:查询合约信息请求ID-%d", t_strLogFuncName, m_nRequestID - 1);
    LOG4CPLUS_INFO(m_objLogger, t_strLog);

    return m_nRequestID - 1;
}

/// 获得委托数据集合的个数
#ifdef MEMORYDATA
int axapi::TradeAPI::sizeOrderList()
{
    return m_vOrderList.size();
}
#endif MEMORYDATA

/// 获得成交数据集合的个数
#ifdef MEMORYDATA
int axapi::TradeAPI::sizeTradeList()
{
    return m_vTradeList.size();
}
#endif MEMORYDATA

/// 获得合约信息数据集合的个数
#ifdef MEMORYDATA
int axapi::TradeAPI::sizeInstrumentList()
{
    return m_vInstrumentList.size();
}
#endif MEMORYDATA

/// 获得持仓明细信息数据集合的个数
#ifdef MEMORYDATA
int axapi::TradeAPI::sizePositionDetailList()
{
    return m_vInvestorPositionDetailList.size();
}
#endif MEMORYDATA

/// 获得合约信息数据集合的个数
#ifdef MEMORYDATA
int axapi::TradeAPI::sizeInstrumentStatusList()
{
    return m_vInstrumentStatusList.size();
}
#endif MEMORYDATA

/// 通过委托遍历位置获得委托信息,in_nOrderListPostion委托遍历位置
#ifdef MEMORYDATA
APINamespace CThostFtdcOrderField axapi::TradeAPI::getOrderInfo(int in_nOrderListPosition)
{
    char* t_strLogFuncName = "TradeAPI::getOrderInfo";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    APINamespace CThostFtdcOrderField t_objOrderInfo;
    memset(&t_objOrderInfo, '\0', sizeof(t_objOrderInfo));
    try {
        if ((unsigned int)in_nOrderListPosition > m_vOrderList.size() || (unsigned int)in_nOrderListPosition <= 0)
        {
            return t_objOrderInfo;
        }
        else
        {
            memcpy_s(&t_objOrderInfo, sizeof(APINamespace CThostFtdcOrderField), &m_vOrderList[in_nOrderListPosition - 1], sizeof(APINamespace CThostFtdcOrderField));
            return t_objOrderInfo;
        }
    }
    catch (std::exception e)
    {
        sprintf_s(t_strLog, 500, "%s:%s", t_strLogFuncName, e.what());
        LOG4CPLUS_FATAL(m_objLogger, t_strLog);
        return t_objOrderInfo;
    }
}
#endif MEMORYDATA

/// 通过成交遍历位置获得成交信息,in_nTradeListPostion成交遍历位置
#ifdef MEMORYDATA
axapi::TradeField axapi::TradeAPI::getTradeInfo(int in_nTradeListPosition)
{
    char* t_strLogFuncName = "TradeAPI::getTradeInfo";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    TradeField t_objTradeInfo;
    memset(&t_objTradeInfo, '\0', sizeof(t_objTradeInfo));
    try {
        if ((unsigned int)in_nTradeListPosition > m_vTradeList.size() || (unsigned int)in_nTradeListPosition <= 0)
        {
            return t_objTradeInfo;
        }
        else
        {
            memcpy_s(&t_objTradeInfo, sizeof(TradeField), &m_vTradeList[in_nTradeListPosition - 1], sizeof(TradeField));
            return t_objTradeInfo;
        }
    }
    catch (std::exception e)
    {
        sprintf_s(t_strLog, 500, "%s:%s", t_strLogFuncName, e.what());
        LOG4CPLUS_FATAL(m_objLogger, t_strLog);
        return t_objTradeInfo;
    }
}
#endif MEMORYDATA

/// 通过合约信息遍历位置获得合约信息,in_nInstrumentListPostion合约遍历位置
#ifdef MEMORYDATA
APINamespace CThostFtdcInstrumentField axapi::TradeAPI::getInstrumentInfo(int in_nInstrumentListPosition)
{
    char* t_strLogFuncName = "TradeAPI::getInstrumentInfo(int)";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    APINamespace CThostFtdcInstrumentField t_objInstrumentInfo;
    memset(&t_objInstrumentInfo, '\0', sizeof(t_objInstrumentInfo));
    try {
        if ((unsigned int)in_nInstrumentListPosition > m_vInstrumentList.size() || (unsigned int)in_nInstrumentListPosition <= 0)
        {
            return t_objInstrumentInfo;
        }
        else
        {
            memcpy_s(&t_objInstrumentInfo, sizeof(t_objInstrumentInfo), &m_vInstrumentList[in_nInstrumentListPosition - 1], sizeof(APINamespace CThostFtdcInstrumentField));
            return t_objInstrumentInfo;
        }
    }
    catch (std::exception e)
    {
        sprintf_s(t_strLog, 500, "%s:%s", t_strLogFuncName, e.what());
        LOG4CPLUS_FATAL(m_objLogger, t_strLog);
        return t_objInstrumentInfo;
    }
}
#endif MEMORYDATA

/// 通过合约代码获得合约信息,in_strContract合约代码
#ifdef MEMORYDATA
APINamespace CThostFtdcInstrumentField axapi::TradeAPI::getInstrumentInfo(char* in_strContract)
{
    char* t_strLogFuncName = "TradeAPI::getInstrumentInfo(char*)";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    APINamespace CThostFtdcInstrumentField t_objInstrumentInfo;
    memset(&t_objInstrumentInfo, '\0', sizeof(t_objInstrumentInfo));
    try {
        for (unsigned int i = 0; i < m_vInstrumentList.size(); i++)
        {
            if (strcmp(m_vInstrumentList[i].InstrumentID, in_strContract) == 0)
            {
                memcpy_s(&t_objInstrumentInfo, sizeof(t_objInstrumentInfo), &m_vInstrumentList[i], sizeof(APINamespace CThostFtdcInstrumentField));
                break;
            }
        }
        return t_objInstrumentInfo;
    }
    catch (std::exception e)
    {
        sprintf_s(t_strLog, 500, "%s:%s", t_strLogFuncName, e.what());
        LOG4CPLUS_FATAL(m_objLogger, t_strLog);
        return t_objInstrumentInfo;
    }
}
#endif MEMORYDATA

/// 通过持仓明细信息遍历位置获得持仓明细信息,in_nPositionDetailListPosition持仓明细遍历位置
#ifdef MEMORYDATA
APINamespace CThostFtdcInvestorPositionDetailField axapi::TradeAPI::getPositionDetailInfo(int in_nPositionDetailListPosition)
{
    char* t_strLogFuncName = "TradeAPI::getPositionDetailInfo";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    APINamespace CThostFtdcInvestorPositionDetailField t_objPositionDetailInfo;
    memset(&t_objPositionDetailInfo, '\0', sizeof(t_objPositionDetailInfo));
    try {
        if ((unsigned int)in_nPositionDetailListPosition > m_vInvestorPositionDetailList.size() || (unsigned int)in_nPositionDetailListPosition <= 0)
        {
            return t_objPositionDetailInfo;
        }
        else
        {
            memcpy_s(&t_objPositionDetailInfo, sizeof(t_objPositionDetailInfo), &m_vInvestorPositionDetailList[in_nPositionDetailListPosition - 1], sizeof(APINamespace CThostFtdcInvestorPositionDetailField));
            return t_objPositionDetailInfo;
        }
    }
    catch (std::exception e)
    {
        sprintf_s(t_strLog, 500, "%s:%s", t_strLogFuncName, e.what());
        LOG4CPLUS_FATAL(m_objLogger, t_strLog);
        return t_objPositionDetailInfo;
    }
}
#endif MEMORYDATA

/// 设置成交信息,主要用于重置成本价格与剩余数量,目前只支持Volumn与Priceset,ret:0 正常
#ifdef MEMORYDATA
int axapi::TradeAPI::setTradeInfo(int in_nTradeListPostion, char* in_Type, double in_dbvalue, int in_nvalue)
{
    char* t_strLogFuncName = "TradeAPI::setTradeInfo";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    try {
        if ((unsigned int)in_nTradeListPostion > m_vTradeList.size() || (unsigned int)in_nTradeListPostion <= 0)
        {
            return -100;
        }
        else
        {
            if (strcmp(in_Type, "Price") == 0)
            {
                m_vTradeList[in_nTradeListPostion - 1].Price = in_dbvalue;
                sprintf_s(t_strLog, 500, "%s:更新m_vTradeList[%d].%s为%f", t_strLogFuncName, in_nTradeListPostion - 1, in_Type, in_dbvalue);
                LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
            }
            else if (strcmp(in_Type, "Volumn") == 0)
            {
                m_vTradeList[in_nTradeListPostion - 1].Volumn = in_nvalue;
                sprintf_s(t_strLog, 500, "%s:更新m_vTradeList[%d].%s为%d", t_strLogFuncName, in_nTradeListPostion - 1, in_Type, in_nvalue);
                LOG4CPLUS_DEBUG(m_objLogger, t_strLog);
            }
            return 0;
        }
    }
    catch (std::exception e)
    {
        sprintf_s(t_strLog, 500, "%s:%s", t_strLogFuncName, e.what());
        LOG4CPLUS_FATAL(m_objLogger, t_strLog);
        return -100;
    }
}
#endif MEMORYDATA

#ifdef MEMORYDATA
/// 通过合约交易状态遍历位置获得合约交易状态,in_nInstrumentStatusListPostion合约遍历位置
APINamespace CThostFtdcInstrumentStatusField axapi::TradeAPI::getInstrumentStatusInfo(int in_nInstrumentStatusListPostion)
{
    char* t_strLogFuncName = "TradeAPI::getInstrumentStatusInfo";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    APINamespace CThostFtdcInstrumentStatusField t_objInstrumentStatusInfo;
    memset(&t_objInstrumentStatusInfo, '\0', sizeof(t_objInstrumentStatusInfo));
    try {
        if ((unsigned int)in_nInstrumentStatusListPostion > m_vInstrumentStatusList.size() || (unsigned int)in_nInstrumentStatusListPostion <= 0)
        {
            return t_objInstrumentStatusInfo;
        }
        else
        {
            memcpy_s(&t_objInstrumentStatusInfo, sizeof(t_objInstrumentStatusInfo), &m_vInstrumentStatusList[in_nInstrumentStatusListPostion - 1], sizeof(APINamespace CThostFtdcInstrumentStatusField));
            return t_objInstrumentStatusInfo;
        }
    }
    catch (std::exception e)
    {
        sprintf_s(t_strLog, 500, "%s:%s", t_strLogFuncName, e.what());
        LOG4CPLUS_FATAL(m_objLogger, t_strLog);
        return t_objInstrumentStatusInfo;
    }
}
#endif MEMORYDATA

/// 通过合约代码获得合约交易状态
#ifdef MEMORYDATA
APINamespace CThostFtdcInstrumentStatusField axapi::TradeAPI::getInstrumentStatusInfo(char* in_strContract)
{
    char* t_strLogFuncName = "TradeAPI::getInstrumentStatusInfo";
    char t_strLog[500];
    sprintf_s(t_strLog, 500, "%s", t_strLogFuncName);
    LOG4CPLUS_TRACE(m_objLogger, t_strLog);

    APINamespace TThostFtdcInstrumentIDType t_strProduct;
    memcpy_s(&t_strProduct, sizeof(t_strProduct), getInstrumentInfo(in_strContract).ProductID, sizeof(APINamespace TThostFtdcInstrumentIDType));

    APINamespace CThostFtdcInstrumentStatusField t_objInstrumentStatusInfo;
    memset(&t_objInstrumentStatusInfo, '\0', sizeof(t_objInstrumentStatusInfo));

    try {
        for (unsigned int i = 0; i < m_vInstrumentStatusList.size(); i++)
        {
            if (strcmp(m_vInstrumentStatusList[i].InstrumentID, t_strProduct) == 0)
            {
                memcpy_s(&t_objInstrumentStatusInfo, sizeof(t_objInstrumentStatusInfo), &m_vInstrumentStatusList[i], sizeof(APINamespace CThostFtdcInstrumentStatusField));
                break;
            }
        }
        return t_objInstrumentStatusInfo;
    }
    catch (std::exception e)
    {
        sprintf_s(t_strLog, 500, "%s:%s", t_strLogFuncName, e.what());
        LOG4CPLUS_FATAL(m_objLogger, t_strLog);
        return t_objInstrumentStatusInfo;
    }
}
#endif MEMORYDATA

/// 获取queryMarketData所查询得到的合约的最新行情
APINamespace CThostFtdcDepthMarketDataField axapi::TradeAPI::getLatestPrice()
{
    return m_LatestPrice;
}