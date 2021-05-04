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

    void ReadMessageStats(Kompex::SQLiteDatabase* pDb);
    bool RecordExists(Kompex::SQLiteDatabase* pDb);
    void Insert(Kompex::SQLiteDatabase* pDb);


public:
    NetworkInfo();

    NetworkInfo(const std::string& name, const std::string& displayName, int protocol);

    void UpdateName(int id);

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
    /// <param name="pDb">Database to update</param>
    static void CleanMessageStatistics(Kompex::SQLiteDatabase* pDb);

    /// <summary>
    /// Inserts info into the Network table or updates it for this network
    /// </summary>
    /// <param name="pDb">Database to update</param>
    void UpdateTable(Kompex::SQLiteDatabase* pDb);

    /// <summary>
    /// Inserts description into the Network table or updates it for this network
    /// </summary>
    /// <param name="pDb">Database to update</param>
    void UpdateDescription(Kompex::SQLiteDatabase* pDb);

    /// <summary>
    /// Returns the network id matching the id found in vsb messages
    /// </summary>
    /// <returns>The network id</returns>
    int GetId() const { return m_id;  }
};

