#pragma once

#include <stdint.h>
#include <map>

#include "NetworkInfo.h"
#include "KompexSQLiteStatement.h"

struct netIdName
{
    char name[30];
    int  id;
    int  vnetOffset;
};

const netIdName networkNames[] = { { "neoVI", 0, 0 },
{ "HS CAN", 1, 1},
{ "MS CAN", 2, 2},
{ "SW CAN", 3, 3},
{ "LSFT CAN", 4, 4},
{ "FORDSCP", 5, 5},
{ "J1708", 6, 6},
{ "AUX", 7, 7},
{ "J1850 VPW", 8, 8},
{ "ISO", 9, 9},
{ "ISOPIC", 10, 10},
{ "MAIN51", 11, 11},
{ "RED", 12, 12},
{ "SCI", 13, 13},
{ "ISO2", 14, 14},
{ "ISO14230", 15, 15},
{ "LIN", 16, 16},
{ "OP (BR) ETH1", 17, 0},
{ "OP (BR) ETH2", 18, 0},
{ "OP (BR) ETH3", 19, 0},
{ "ISO3", 41, 17},
{ "HS CAN2", 42, 18},
{ "HS CAN3", 44, 19},
{ "OP (BR) ETH4", 45, 0},
{ "OP (BR) ETH5", 46, 0},
{ "ISO4", 47, 20},
{ "LIN2", 48, 21},
{ "LIN3", 49, 22},
{ "LIN4", 50, 23},
{ "MOST", 51, 24},
{ "RED_APP_ERROR", 52, 46},
{ "CGI", 53, 25},
{ "3G_RESET_STATUS", 54, 0},
{ "3G_FB_STATUS", 55, 0},
{ "3G_APP_SIGNAL_STATUS", 56, 0},
{ "3G_READ_DATALINK_CM_TX_MSG", 57, 0},
{ "3G_READ_DATALINK_CM_RX_MSG", 58, 0},
{ "3G_LOGGING_OVERFLOW", 59, 0},
{ "3G_READ_SETTINGS_EX", 60, 0},
{ "HS CAN4", 61, 32},
{ "HS CAN5", 62, 33},
{ "RS232", 63, 34},
{ "UART", 64, 35},
{ "UART2", 65, 36},
{ "UART3", 66, 37},
{ "UART4", 67, 38},
{ "SW CAN2", 68, 39},
{ "Ethernet DAQ", 69, 45},
{ "DATA_TO_HOST", 70},
{ "I2C1", 71, 26},
{ "SPI1", 72, 27},
{ "OP (BR) ETH6", 73, 0},
{ "RED_VBAT", 74, 0},
{ "OP (BR) ETH7", 75, 0},
{ "OP (BR) ETH8", 76, 0},
{ "OP (BR) ETH9", 77, 0},
{ "OP (BR) ETH10", 78, 0},
{ "OP (BR) ETH11", 79, 0},
{ "FlexRay1A", 80, 28},
{ "FlexRay1B", 81, 40},
{ "FlexRay2A", 82, 41},
{ "FlexRay2B", 83, 42},
{ "LIN5", 84, 43},
{ "FlexRay", 85, 0},
{ "FlexRay2", 86, 0},
{ "OP (BR) ETH12", 87, 0},
{ "MOST25", 90, 29},
{ "MOST50", 91, 30},
{ "MOST150", 92, 31},
{ "Ethernet", 93, 44},
{ "GMFSA", 94, 0},
{ "TCP", 95, 0},
{ "HS CAN6", 96, 47},
{ "HS CAN7", 97, 48},
{ "LIN6", 98, 49},
{ "LSFT CAN2", 99, 50 } };

const char protocols[][20] = { "CUSTOM",
"CAN",
"GMLAN",
"J1850VPW",
"J1850PWM",
"ISO9141",
"Keyword2000",
"GM_ALDL_UART",
"CHRYSLER_CCD",
"CHRYSLER_SCI",
"FORD_UBP",
"BEAN",
"LIN",
"J1708",
"CHRYSLER_JVPW",
"J1939",
"FLEXRAY",
"MOST",
"CGI",
"GME_CIM_SCL_KLINE",
"SPI",
"I2C",
"GENERIC_UART",
"JTAG",
"UNIO",
"DALLAS_1WIRE",
"GENERIC_MANCHESTER",
"SENT_PROTOCOL",
"UART",
"ETHERNET",
"CANFD",
"GMFSA",
"TCP" };

class VSBInfo
{
    std::map<int, NetworkInfo> m_mapNetworks;
    std::multimap<uint64_t, std::vector<unsigned char> > messageCache;
    Kompex::SQLiteDatabase* m_pDb;

    void FlushCache(size_t numToFlush = 0);
    void SaveMessage(Kompex::SQLiteStatement& insertMessage, uint64_t timestamp, const std::vector<unsigned char>& data);

public:

    VSBInfo(Kompex::SQLiteDatabase* pDb);
    ~VSBInfo();

    /// <summary>
    /// Reads the message timestamp, updates the network info, adds it to the sorted cache and
    /// when the cache gets full, it empties half of it
    /// </summary>
    /// <param name="data"></param>
    void ProcessMessage(const std::vector<unsigned char>& data);

    /// <summary>
    /// Writes the network summary table info.
    /// </summary>
    void UpdateTable()
    {
        FlushCache();

        for (std::map<int, NetworkInfo>::iterator it = m_mapNetworks.begin(); it != m_mapNetworks.end(); ++it)
        {
            (*it).second.UpdateTable(m_pDb);
        }
    }

    /// <summary>
    /// Removes the old data tables if they exist and re-creates them
    /// </summary>
    void CleanTables(bool bAppend);

};

/// <summary>
/// Creates a Sqlite database containing all the messages in the vsb file.
/// </summary>
/// <param name="pVsbPath">File for which to generate a database</param>
/// <param name="pDbPath">File to generate</param>
/// <param name="bAppend">Append to or re-create the data tables</param>
/// <param name="prog">Progess callback</param>
/// <returns>Whether the database was created</returns>
bool CreateDb(const char* pVsbPath, const char* pDbPath, bool bAppend, ProgressFunc prog);

/// <summary>
/// Creates a vsb file from the given Sqlite message database.
/// The filter is basically the WHERE clause, which can restrict messages by columns such as MessageTime, NetworkId, Id, etc
/// Please consult the database schema for details.
/// </summary>
/// <param name="pDbPath">Database from which the messages will be read</param>
/// <param name="pVsbPath">Output file containing filtered messages</param>
/// <param name="pFilter">The WHERE clause, which can be used to filter a subset of the messages</param>
/// <param name="prog">Progess callback</param>
/// <returns>The number of records written or -1 if an error occurred</returns>
int WriteVsb(const char* pDbPath, const char* pVsbPath, const char* pFilter, ProgressFunc prog);
