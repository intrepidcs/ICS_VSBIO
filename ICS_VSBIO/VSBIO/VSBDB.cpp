#include <string.h>

#include "VSBIODLL.h"
#include "VSBIO.h"
#include "OFile.h"
#include "MessageTimeDecoderVSB.h"
#include "VSBDB.h"
#include "KompexSQLiteException.h"

using namespace Kompex;

#if defined _WIN32
#include <tchar.h>
#endif

#define DROP_NETWORK "DROP TABLE IF EXISTS Network"
#define CREATE_NETWORK "CREATE TABLE Network (Id INTEGER, Name TEXT NOT NULL, Device TEXT, Protocol TEXT NOT NULL, DisplayName TEXT NOT NULL," \
        "FirstTime INTEGER, LastTime INTEGER, NumMessages INTEGER, PRIMARY KEY(Id))"
#define INSERT_NETWORK "INSERT INTO Network (Id, Name, Device, Protocol, DisplayName, FirstTime, LastTime, NumMessages) VALUES " \
        "(@Id, @Name, @Device, @Protocol, @DisplayName, @FirstTime, @LastTime, @NumMessages)"

#define DROP_MESSAGES "DROP TABLE IF EXISTS RawMessageData"
#define CREATE_MESSAGES "CREATE TABLE RawMessageData (MessageTime INTEGER, NetworkId INTEGER, Id INTEGER, Status INTEGER NOT NULL, Ack INTEGER, " \
        "Header INTEGER, DataSize INTEGER, Data BLOB, FOREIGN KEY(NetworkId) REFERENCES Network(Id) ON DELETE CASCADE, " \
        "PRIMARY KEY(MessageTime,NetworkId,Id) )"
#define INSERT_MESSAGES "INSERT OR IGNORE INTO RawMessageData (MessageTime, NetworkId, Id, Status, Ack, Header, DataSize, Data) VALUES " \
        "(@MessageTime, @NetworkId, @Id, @Status, @Ack, @Header, @DataSize, @Data)"

#define SELECT_MESSAGES "SELECT MessageTime, NetworkId, Id, Status, Ack, Header, DataSize, Data FROM RawMessageData"
const int DataSize_Read_Offset = 6;  // 0 index in previous select

/// <summary>
/// Writes the network summary table info. Id -1 is the whole file summary
/// </summary>
/// <param name="pDb">Database to work with</param>
void NetworkInfo::SaveInfo(SQLiteDatabase *pDb) const
{
    SQLiteStatement insertNetworks(pDb);
    insertNetworks.Sql(INSERT_NETWORK);
    string netName, protocol;
    if ((m_id < 0) || (m_id >= NETID_MAX))
        netName = "Unknown";
    else
    {
        netName = networkNames[m_id];
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
    }
    int nCol = 1;
    insertNetworks.BindInt(nCol++, m_id);
    insertNetworks.BindString(nCol++, netName);
    insertNetworks.BindString(nCol++, "");
    insertNetworks.BindString(nCol++, protocol);
    insertNetworks.BindString(nCol++, netName);
    insertNetworks.BindInt64(nCol++, m_firstTime);
    insertNetworks.BindInt64(nCol++, m_lastTime);
    insertNetworks.BindInt64(nCol++, m_numMessages);
    insertNetworks.ExecuteAndFree();
}

/// <summary>
/// Re-creates the two tables we need in the database
/// </summary>
/// <param name="pDb">Database object to work with</param>
VSBInfo::VSBInfo(SQLiteDatabase* pDb)
{
    m_pDb = pDb;
    SQLiteStatement stmtDbSetup(m_pDb);
    stmtDbSetup.SqlStatement("PRAGMA journal_mode=OFF");       // we don't roll back transactions so no journal required
    stmtDbSetup.SqlStatement("PRAGMA synchronous=OFF");        // let the OS handle writes, no need to protect against power failures
    stmtDbSetup.SqlStatement("PRAGMA count_changes=OFF");      // this is a deprecated pragma but can help performance if followed
    stmtDbSetup.SqlStatement("PRAGMA cache_size=100000");      // this should max out at just over 80MB of max memory
    stmtDbSetup.SqlStatement("PRAGMA locking_mode=EXCLUSIVE"); // only one reader/writer to the db we're creating

    stmtDbSetup.SqlStatement(DROP_NETWORK);
    stmtDbSetup.SqlStatement(CREATE_NETWORK);

    stmtDbSetup.SqlStatement(DROP_MESSAGES);
    stmtDbSetup.SqlStatement(CREATE_MESSAGES);
}

/// <summary>
/// Destructor, inserts the rest of the cache and cleans up the database
/// </summary>
VSBInfo::~VSBInfo()
{ 
    FlushCache(); 
    SQLiteStatement stmtDbCleanup(m_pDb);
    stmtDbCleanup.SqlStatement("PRAGMA journal_mode=DELETE");
    stmtDbCleanup.SqlStatement("PRAGMA synchronous=FULL");
    stmtDbCleanup.SqlStatement("PRAGMA shrink_memory");
}

/// <summary>
/// Saves one message to the database using the prepared statement provided
/// </summary>
/// <param name="insertMessage">Prepared statement</param>
/// <param name="timestamp">Message timestamp in time64_t format</param>
/// <param name="data">Message and bytes</param>
void VSBInfo::SaveMessage(SQLiteStatement &insertMessage, uint64_t timestamp, const std::vector<unsigned char>& data)
{
    icsSpyMessageVSB* msg = ((icsSpyMessageVSB*)&data[0]);
    int nCol = 1;
    insertMessage.BindInt64(nCol++, timestamp);
    insertMessage.BindInt(nCol++, (((uint32_t)msg->NetworkID2) << 8) | (uint32_t)msg->NetworkID);
    insertMessage.BindInt(nCol++, msg->ArbIDOrHeader);
    insertMessage.BindInt64(nCol++, (((uint64_t)msg->StatusBitField2) << 32) | (uint64_t)msg->StatusBitField);
    insertMessage.BindInt64(nCol++, *((uint64_t *)&msg->AckBytes));
    insertMessage.BindInt64(nCol++, (((uint64_t)msg->MessagePieceID << 16) | (uint64_t)msg->Protocol << 8) | (uint64_t)msg->NumberBytesHeader);

    if (data.size() > sizeof(icsSpyMessageVSB)) // If the message has more than 8 bytes, it's saved after the struct
    {
        insertMessage.BindInt(nCol++, (int)(data.size() - sizeof(icsSpyMessageVSB)));
        insertMessage.BindBlob(nCol++, &data[sizeof(icsSpyMessageVSB)], (int)(data.size() - sizeof(icsSpyMessageVSB)));
    }
    else // The data bytes are in the data field
    {
        insertMessage.BindInt(nCol++, msg->NumberBytesData);
        insertMessage.BindBlob(nCol++, &msg->Data, msg->NumberBytesData);
    }
    insertMessage.Execute();
    insertMessage.Reset();
}

/// <summary>
/// Flushes the messages requested to the dabase
/// </summary>
/// <param name="numToFlush">Number of messages to flush</param>
void VSBInfo::FlushCache(size_t numToFlush)
{
    SQLiteStatement insertMessage(m_pDb);
    insertMessage.BeginTransaction();
    insertMessage.Sql(INSERT_MESSAGES);
    int nCurItem = 0;
    for (std::multimap<uint64_t, std::vector<unsigned char> >::iterator itMsg = messageCache.begin(); itMsg != messageCache.end(); ++itMsg, ++nCurItem)
    {
        SaveMessage(insertMessage, itMsg->first, itMsg->second);
        if (numToFlush && (nCurItem >= numToFlush))
        {
            messageCache.erase(messageCache.begin(), itMsg);
            break;
        }
    }
    if (numToFlush == 0)  // Remaining messages
        messageCache.clear();
    insertMessage.CommitTransaction();
}

/// <summary>
/// Reads the message timestamp, updates the network info, adds it to the sorted cache and
/// when the cache gets full, it empties half of it
/// </summary>
/// <param name="data"></param>
void VSBInfo::ProcessMessage(const std::vector<unsigned char>& data)
{
    icsSpyMessageVSB* msg = ((icsSpyMessageVSB*)&data[0]);
    uint64_t timestamp = CMessageTimeDecoderVSB::CalcEpoch64(*msg);
    uint32_t networkId = (((uint32_t)msg->NetworkID2) << 8) | (uint32_t)msg->NetworkID;

    // Update the network statistics
    m_mapNetworks[msg->NetworkID].ProcessMessage(timestamp, networkId, msg->Protocol);

    messageCache.insert(std::make_pair(timestamp, data));  // Add to sorted container
    if (messageCache.size() >= 100000)           // Flush half the sorted container
        FlushCache(messageCache.size() / 2);
}

/// <summary>
/// Reads a message from the Statement
/// </summary>
/// <param name="qryMessages">Statement to read from</param>
/// <param name="data">Byte array set with vsb message to write</param>
/// <returns>Number of bytes to write so the vector isn't shrunk needlessly</returns>
unsigned int ReadMessage(const SQLiteStatement &qryMessages, std::vector<unsigned char> &data)
{
    uint64_t dataSize = qryMessages.GetColumnInt64(DataSize_Read_Offset);

    size_t msgSize = sizeof(icsSpyMessageVSB);
    if (dataSize > 8)
        msgSize += dataSize;
    if (data.size() < msgSize)
        data.resize(msgSize);

    icsSpyMessageVSB* msg = ((icsSpyMessageVSB*)&data[0]);
    memset(msg, 0, msgSize);
    int nCol = 0;
    CMessageTimeDecoderVSB::SetMessageTime(*msg, qryMessages.GetColumnInt64(nCol++));
    uint32_t networkId = qryMessages.GetColumnInt(nCol++);
    msg->NetworkID2 = (uint8_t)(networkId >> 8);
    msg->NetworkID = (uint8_t)(networkId & 0xff);
    msg->ArbIDOrHeader = qryMessages.GetColumnInt(nCol++);
    uint64_t status = qryMessages.GetColumnInt64(nCol++);
    msg->StatusBitField2 = (uint32_t)(status >> 32);
    msg->StatusBitField = (uint32_t)(status & 0xffffffff);
    *((uint64_t*)&msg->AckBytes) = qryMessages.GetColumnInt64(nCol++);
    uint64_t protocol = qryMessages.GetColumnInt64(nCol++);
    msg->MessagePieceID = (uint8_t)(protocol >> 16);
    msg->Protocol = (uint8_t)((protocol >> 8) & 0xff);
    msg->NumberBytesHeader = (uint8_t)(protocol & 0xff);

    nCol++; // Data size had to be read first
    if (dataSize <= 8)
    {
        msg->NumberBytesData = (uint8_t)dataSize;
        if (dataSize)
            memcpy(msg->Data.data, qryMessages.GetColumnBlob(nCol++), dataSize);
    }
    else
    {
        msg->ExtraDataPtrEnabled = 1;
        msg->ExtraDataPtr = (uint32_t)dataSize;
        memcpy(&data[sizeof(icsSpyMessageVSB)], qryMessages.GetColumnBlob(nCol++), dataSize);
    }
    return (unsigned int)msgSize;
}

/// <summary>
/// Creates a Sqlite database containing all the messages in the vsb file.
/// </summary>
/// <param name="pVsbPath">File for which to generate a database</param>
/// <param name="pDbPath">File to generate</param>
/// <param name="prog">Progess callback</param>
/// <returns>Whether the database was created</returns>
bool CreateDb(const char *pVsbPath, const char *pDbPath, ProgressFunc prog)
{
    try
    {
        SQLiteDatabase db(pDbPath, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, 0);
        VSBIORead read(pVsbPath);
        std::vector<unsigned char> msg;
        VSBInfo info(&db);
        uint64_t counter = 0;
        while (read.ReadNextMessage(msg) == VSBIORead::eSuccess)
        {
            if (prog && !((counter++) % 100000))
            {
                if (!prog(read.GetProgress()))
                    break;
            }
            info.ProcessMessage(msg);
        }
        info.SaveInfo();
        return true;
    }
    catch (SQLiteException& exception)
    {
        exception.Show();
        return false;
    }
}

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
bool WriteVsb(const char* pDbPath, const char* pVsbPath, const char *pFilter, ProgressFunc prog)
{
    try
    {
        SQLiteDatabase db(pDbPath, SQLITE_OPEN_READONLY, 0);

        string sSql(SELECT_MESSAGES);
        if (pFilter && *pFilter)
        {
            sSql += " WHERE ";
            sSql += pFilter;
        }
        SQLiteStatement qryMessages(&db);
        qryMessages.Sql(sSql);

        VSBIOWrite write;
        write.Init(pVsbPath);

        std::vector<unsigned char> msg;
        uint64_t counter = 0;
        int pct = 0;
        unsigned int messageSize;
        while (qryMessages.FetchRow())
        {
            if (prog && !((counter + 1) % 100000))
            {
                if (!prog(++pct))
                    break;
            }
            messageSize = ReadMessage(qryMessages, msg);
            write.WriteMessage(&msg[0], messageSize);
        }

        return true;
    }
    catch (SQLiteException& exception)
    {
        exception.Show();
        return false;
    }
}