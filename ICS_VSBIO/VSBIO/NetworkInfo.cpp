#include <string.h>

#include "VSBIODLL.h"
#include "VSBIO.h"
#include "OFile.h"
#include "MessageTimeDecoderVSB.h"
#include "VSBDatabase.h"
#include "KompexSQLiteException.h"

using namespace Kompex;

#define CLEAR_MESSAGE_STATS "UPDATE Network SET FirstTime = 0, LastTime = 0, NumMessages = 0"
#define CREATE_NETWORK "CREATE TABLE IF NOT EXISTS Network (Id INTEGER, Name TEXT NOT NULL, Device TEXT, Protocol TEXT NOT NULL, DisplayName TEXT NOT NULL," \
        "FirstTime INTEGER, LastTime INTEGER, NumMessages INTEGER, PRIMARY KEY(Id))"
#define INSERT_NETWORK "INSERT OR IGNORE INTO Network (Id, Name, Device, Protocol, DisplayName, FirstTime, LastTime, NumMessages) VALUES " \
        "(@Id, @Name, @Device, @Protocol, @DisplayName, @FirstTime, @LastTime, @NumMessages)"
#define UPDATE_MESSAGE_STATS "UPDATE Network SET FirstTime = @FirstTime, LastTime = @LastTime, NumMessages = @NumMessages WHERE Id = @Id"
#define UPDATE_DESCRIPTION "UPDATE Network SET Name = @Name, DisplayName = @DisplayName WHERE Id = @Id"

#define READ_NETWORK "SELECT * FROM Network WHERE Id = @Id"
#define GET_NETWORK_INFO "SELECT Min(MessageTime), Max(MessageTime), Count(*) FROM RawMessageData WHERE NetworkId = @Id"

/// <summary>
/// Constructor for use when we are adding decodings and need to find the network id
/// </summary>
/// <param name="name">Internal network name</param>
/// <param name="displayName">User-specified network name</param>
/// <param name="protocol">Protocol for this network</param>
NetworkInfo::NetworkInfo(const std::string& name, const std::string& displayName, int protocol) : m_id(-1), m_name(name), m_displayName(displayName), m_firstTime(0), m_lastTime(0),
m_numMessages(0)
{
    size_t strip = m_name.find(" (neoVI 3G)");
    if (strip != string::npos)
        m_name = m_name.substr(0, strip);

    int vnetOffset = 0;
    string findName = m_name;
    strip = findName.find(" (VNET A)");
    if (strip != string::npos)
    {
        findName = findName.substr(0, strip);
        vnetOffset = PLASMA_SLAVE1_OFFSET;
    }
    else
    {
        strip = findName.find(" (VNET B)");
        if (strip != string::npos)
        {
            findName = findName.substr(0, strip);
            vnetOffset = PLASMA_SLAVE2_OFFSET;
        }
    }
    for (size_t nCnt = 0; nCnt < sizeof(networkNames) / sizeof(networkNames[0]); ++nCnt)
    {
        if (networkNames[nCnt].name == findName)
        {
            if (vnetOffset)
                m_id = networkNames[nCnt].vnetOffset + vnetOffset;
            else
                m_id = networkNames[nCnt].id;
            break;
        }
    }
    m_protocol.insert(protocol);
}

/// <summary>
/// Default constructor
/// </summary>
NetworkInfo::NetworkInfo()
{
    m_id = -1;
    m_firstTime = m_lastTime = 0;
    m_numMessages = 0;
}

void NetworkInfo::ProcessMessage(uint64_t timestamp, int id, int protocol)
{
    m_protocol.insert(protocol);
    if (m_firstTime == 0)
    {
        m_firstTime = timestamp;
        UpdateName(id);
    }
    else if (m_firstTime > timestamp)
        m_firstTime = timestamp;

    if (m_lastTime < timestamp)
        m_lastTime = timestamp;
    ++m_numMessages;
}

/// <summary>
/// The first time we have this network's id, we will look up its internal name
/// </summary>
/// <param name="id">Message network id</param>
void NetworkInfo::UpdateName(int id)
{
    if (id != m_id)
    {
        m_id = id;
        for (size_t nCnt = 0; nCnt < sizeof(networkNames) / sizeof(networkNames[0]); ++nCnt)
        {
            if (m_id == networkNames[nCnt].id)
            {
                m_name = networkNames[nCnt].name;
                break;
            }
            if (m_id == networkNames[nCnt].vnetOffset + PLASMA_SLAVE1_OFFSET)
            {
                m_name = networkNames[nCnt].name;
                m_name += " (VNET A)";
                break;
            }
            if (m_id == networkNames[nCnt].vnetOffset + PLASMA_SLAVE2_OFFSET)
            {
                m_name = networkNames[nCnt].name;
                m_name += " (VNET B)";
                break;
            }
        }
        m_displayName = m_name;
    }

}

/// <summary>
/// Aggregates the message stats for this network.  This is needed because sqlite does not currently support UPDATE subqueries.
/// </summary>
/// <param name="pDb">Database to update</param>
void NetworkInfo::ReadMessageStats(SQLiteDatabase* pDb)
{
    SQLiteStatement qryNetwork(pDb);
    qryNetwork.Sql(GET_NETWORK_INFO);
    qryNetwork.BindInt64(1, m_id);

    if (qryNetwork.FetchRow())  // Update the stats after possibly appending messages
    {
        int nQryCol = 0;
        m_firstTime = qryNetwork.GetColumnInt64(nQryCol++);
        m_lastTime = qryNetwork.GetColumnInt64(nQryCol++);
        m_numMessages = qryNetwork.GetColumnInt64(nQryCol++);
    }
}

/// <summary>
/// Returns true if a row exists for this network.  This is to preserve the names and fields updated elsewhere.
/// </summary>
/// <param name="pDb">Database to update</param>
/// <returns>true if a record for this network exists</returns>
bool NetworkInfo::RecordExists(SQLiteDatabase* pDb)
{
    SQLiteStatement qryFind(pDb);
    qryFind.Sql(READ_NETWORK);
    qryFind.BindInt64(1, m_id);

    return qryFind.FetchRow();
}

/// <summary>
/// Inserts the record, once we made sure it doesn't exist
/// </summary>
/// <param name="pDb">Database to update</param>
void NetworkInfo::Insert(SQLiteDatabase* pDb)
{
    string protocol;
    // CAN and CAN FD for example
    for (std::set<int>::const_iterator itProtocol = m_protocol.begin(); itProtocol != m_protocol.end(); ++itProtocol)
    {
        if ((*itProtocol >= 0) && (*itProtocol <= SPY_PROTOCOL_TCP))
        {
            if (protocol.size())
                protocol += ",";
            protocol += protocols[*itProtocol];
        }
    }

    SQLiteStatement insertNetworks(pDb);
    insertNetworks.Sql(INSERT_NETWORK);
    int nCol = 1;
    insertNetworks.BindInt(nCol++, m_id);
    insertNetworks.BindString(nCol++, m_name);
    insertNetworks.BindString(nCol++, "");
    insertNetworks.BindString(nCol++, protocol);
    insertNetworks.BindString(nCol++, m_displayName);
    insertNetworks.BindInt64(nCol++, m_firstTime);
    insertNetworks.BindInt64(nCol++, m_lastTime);
    insertNetworks.BindInt64(nCol++, m_numMessages);
    insertNetworks.ExecuteAndFree();
}

void NetworkInfo::UpdateTable(SQLiteDatabase *pDb)
{
    if (RecordExists(pDb))
    {
        ReadMessageStats(pDb);
        SQLiteStatement updateMessageStats(pDb);
        updateMessageStats.Sql(UPDATE_MESSAGE_STATS);
        int nCol = 1;
        updateMessageStats.BindInt64(nCol++, m_firstTime);
        updateMessageStats.BindInt64(nCol++, m_lastTime);
        updateMessageStats.BindInt64(nCol++, m_numMessages);
        updateMessageStats.BindInt(nCol++, m_id);
        updateMessageStats.ExecuteAndFree();

    }
    else
    {
        Insert(pDb);
    }
}

void NetworkInfo::UpdateDescription(SQLiteDatabase* pDb)
{
    if (RecordExists(pDb))
    {
        ReadMessageStats(pDb);
        SQLiteStatement updateMessageStats(pDb);
        updateMessageStats.Sql(UPDATE_DESCRIPTION);
        int nCol = 1;
        updateMessageStats.BindString(nCol++, m_name);
        updateMessageStats.BindString(nCol++, m_displayName);
        updateMessageStats.BindInt(nCol++, m_id);
        updateMessageStats.ExecuteAndFree();

    }
    else
    {
        Insert(pDb);
    }
}

void NetworkInfo::CleanMessageStatistics(Kompex::SQLiteDatabase* pDb)
{
    SQLiteStatement stmtDbClean(pDb);
    stmtDbClean.SqlStatement(CREATE_NETWORK);
    stmtDbClean.SqlStatement(CLEAR_MESSAGE_STATS);
}
