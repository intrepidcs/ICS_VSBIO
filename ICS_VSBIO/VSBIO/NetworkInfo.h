#pragma once

#include <stdint.h>
#include <set>

#include "KompexSQLiteDatabase.h"

class NetworkInfo
{
    int m_id;
    std::set<int> m_protocol;
    uint64_t m_firstTime, m_lastTime;
    uint64_t m_numMessages;
    std::string m_name, m_displayName;

    void ReadMessageStats(Kompex::SQLiteDatabase* pMessageDb);
    bool RecordExists(Kompex::SQLiteDatabase* pMessageDb);
    void Insert(Kompex::SQLiteDatabase* pMessageDb);

public:
    NetworkInfo();

    NetworkInfo(const std::string& name, const std::string& displayName, int protocol);

    // Old databases didn't have the network info
    bool UpdateFromKey(const std::string& networkKey);

    bool HasProtocol(int protocol) { return (m_protocol.find(protocol) != m_protocol.end()); }

    void UpdateName(int id);

    static int GetIdFromName(const std::string& name);

    /// <summary>
    /// Updates the messages statistics
    /// </summary>
    /// <param name="timestamp">Message timestamp</param>
    /// <param name="id">Message id</param>
    /// <param name="protocol">Message protocol</param>
    void ProcessMessage(uint64_t timestamp, int id = -1, int protocol = -1);

    /// <summary>
    /// Makes sure the Network table exists and cleans up the network first and last time and message count
    /// </summary>
    /// <param name="pMessageDb">Database to update</param>
    static void CleanMessageStatistics(Kompex::SQLiteDatabase* pMessageDb);

    /// <summary>
    /// Inserts info into the Network table or updates it for this network
    /// </summary>
    /// <param name="pMessageDb">Database to update</param>
    void UpdateTable(Kompex::SQLiteDatabase* pMessageDb);

    /// <summary>
    /// Inserts description into the Network table or updates it for this network
    /// </summary>
    /// <param name="pMessageDb">Database to update</param>
    void UpdateDescription(Kompex::SQLiteDatabase* pMessageDb);

    /// <summary>
    /// Returns the network id matching the id found in vsb messages
    /// </summary>
    /// <returns>The network id</returns>
    int GetId() const { return m_id;  }

    std::string GetName() const { return m_name; }
};

