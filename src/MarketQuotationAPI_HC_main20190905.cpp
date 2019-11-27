#define OTL_ORA10G
#define OTL_STREAM_READ_ITERATOR_ON
#include <otlv4\otlv4.h>
#include <iostream>
#include <vector>

otl_connect g_dblink;
std::vector<int> g_array;

void select()
{
    int t_value;
    otl_stream selectdata(50, "select * from axadmin.tmp_table", g_dblink);
    /*while (!selectdata.eof())
    {
        selectdata >> t_value;
        g_array.push_back(t_value);
        if (g_array.size() % 1000 == 0)
        {
            std::cout << g_array.size() << " data has been loaded." << std::endl;
        }
    }
    std::cout << g_array.size() << " data has been loaded." << std::endl;*/

    otl_stream_read_iterator<otl_stream, otl_exception, otl_lob_stream> ri;
    ri.attach(selectdata);
    while (ri.next_row())
    {
        ri.get(1, t_value);
        std::cout << t_value << std::endl;
    }
    ri.detach();
}

void insert()
{

    otl_stream insertdata(50, "insert into axadmin.tmp_table values(:f1<int>)", g_dblink);
    for (int i = 0; i < 50000; i++)
    {
        if (i % 1000 == 0)
        {
            std::cout << i << " data has been inserted." << std::endl;
        }
        insertdata << i;
    }
}

void main_20190905()
{
    try {
        std::cout << "before rlogon(), dblink.connected : " << (g_dblink.connected == 0 ? "Disconnected" : "Connected") << std::endl;
        g_dblink.rlogon("axadmin/axadmin@headdbv6", 1);
        std::cout << "after rlogon(), dblink.connected : " << (g_dblink.connected == 0 ? "Disconnected" : "Connected") << std::endl;

        insert();
        select();

        g_dblink.logoff();
        std::cout << "after logoff(), dblink.connected : " << (g_dblink.connected == 0 ? "Disconnected" : "Connected") << std::endl;
    }
    catch (otl_exception e)
    {
        std::cout << e.code << ":" << e.msg << std::endl;
    }
}