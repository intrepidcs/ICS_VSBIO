#pragma once

#include <stdint.h>
#include <map>

#include "NetworkLookups.h"
#include "NetworkInfo.h"
#include "VSBStruct.h"
#include "KompexSQLiteStatement.h"

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
    void UpdateTable();

    /// <summary>
    /// Removes the old data tables if they exist and re-creates them
    /// </summary>
    void CleanTables(bool bAppend);

};

void PrepareForWriting(Kompex::SQLiteDatabase* pDb);
void CleanAfterWriting(Kompex::SQLiteDatabase* pDb);

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
