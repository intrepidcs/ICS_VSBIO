#pragma once

#include <stdint.h>
#include <map>
#include <set>

#include "KompexSQLiteDatabase.h"
#include "KompexSQLiteStatement.h"

struct netIdName
{
    char name[30];
    int  id;
};

const netIdName networkNames[] = { { "neoVI", 0 },
{ "HSCAN", 1},
{ "MSCAN", 2},
{ "SWCAN", 3},
{ "LSFTCAN", 4},
{ "FORDSCP", 5},
{ "J1708", 6},
{ "AUX", 7},
{ "JVPW", 8},
{ "ISO", 9},
{ "ISOPIC", 10},
{ "MAIN51", 11},
{ "RED", 12},
{ "SCI", 13},
{ "ISO2", 14},
{ "ISO14230", 15},
{ "LIN", 16},
{ "OP_ETHERNET1", 17},
{ "OP_ETHERNET2", 18},
{ "OP_ETHERNET3", 19},
{ "ISO3", 41},
{ "HSCAN2", 42},
{ "HSCAN3", 44},
{ "OP_ETHERNET4", 45},
{ "OP_ETHERNET5", 46},
{ "ISO4", 47},
{ "LIN2", 48},
{ "LIN3", 49},
{ "LIN4", 50},
{ "MOST", 51},
{ "RED_APP_ERROR", 52},
{ "CGI", 53},
{ "3G_RESET_STATUS", 54},
{ "3G_FB_STATUS", 55},
{ "3G_APP_SIGNAL_STATU", 56},
{ "3G_READ_DATALINK_CM_TX_MSG", 57},
{ "3G_READ_DATALINK_CM_RX_MSG", 58},
{ "3G_LOGGING_OVERFLOW", 59},
{ "3G_READ_SETTINGS_EX", 60},
{ "HSCAN4", 61},
{ "HSCAN5", 62},
{ "RS232", 63},
{ "UART", 64},
{ "UART2", 65},
{ "UART3", 66},
{ "UART4", 67},
{ "SWCAN2", 68},
{ "ETHERNET_DAQ", 69},
{ "DATA_TO_HOST", 70},
{ "I2C1", 71},
{ "SPI1", 72},
{ "OP_ETHERNET6", 73},
{ "RED_VBAT", 74},
{ "OP_ETHERNET7", 75},
{ "OP_ETHERNET8", 76},
{ "OP_ETHERNET9", 77},
{ "OP_ETHERNET10", 78},
{ "OP_ETHERNET11", 79},
{ "FLEXRAY1A", 80},
{ "FLEXRAY1B", 81},
{ "FLEXRAY2A", 82},
{ "FLEXRAY2B", 83},
{ "LIN5", 84},
{ "FLEXRAY", 85},
{ "FLEXRAY2", 86},
{ "OP_ETHERNET12", 87},
{ "MOST25", 90},
{ "MOST50", 91},
{ "MOST150", 92},
{ "ETHERNET", 93},
{ "GMFSA", 94},
{ "TCP", 95},
{ "HSCAN6", 96},
{ "HSCAN7", 97},
{ "LIN6", 98},
{ "LSFTCAN2", 99 } };

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



class NetworkInfo
{
    int m_id;
    std::set<int> m_protocol;
    uint64_t m_firstTime, m_lastTime;
    uint64_t m_numMessages;
public:
    NetworkInfo()
    {
        m_id = -1;
        m_firstTime = m_lastTime = 0;
        m_numMessages = 0;
    }
    void ProcessMessage(uint64_t timestamp, int id = -1, int protocol = -1)
    {
        m_protocol.insert(protocol);
        if (m_firstTime == 0)
        {
            m_firstTime = timestamp;
            m_id = id;
        }
        else if (m_firstTime > timestamp)
            m_firstTime = timestamp;

        if (m_lastTime < timestamp)
            m_lastTime = timestamp;
        ++m_numMessages;
    }

    void SaveInfo(Kompex::SQLiteDatabase* pDb) const;
};


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
    /// Writes the network summary table info. Id -1 is the whole file summary
    /// </summary>
    /// <param name="pDb">Database to work with</param>
    void SaveInfo() const
    {
        for (std::map<int, NetworkInfo>::const_iterator it = m_mapNetworks.begin(); it != m_mapNetworks.end(); ++it)
        {
            (*it).second.SaveInfo(m_pDb);
        }
    }

    /// <summary>
    /// Removes the old data tables if they exist and re-creates them
    /// </summary>
    void CleanTables();

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
/// <returns>Whether the VSB file was created</returns>
bool WriteVsb(const char* pDbPath, const char* pVsbPath, const char* pFilter, ProgressFunc prog);
