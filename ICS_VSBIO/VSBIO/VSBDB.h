#pragma once

#include <stdint.h>
#include <map>
#include <set>

#include "KompexSQLiteDatabase.h"
#include "KompexSQLiteStatement.h"

const char networkNames[][30] = { "neoVI",
"HSCAN",
"MSCAN",
"SWCAN",
"LSFTCAN",
"FORDSCP",
"J1708",
"AUX",
"JVPW",
"ISO",
"ISOPIC",
"MAIN51",
"RED",
"SCI",
"ISO2",
"ISO14230",
"LIN",
"OP_ETHERNET1",
"OP_ETHERNET2",
"OP_ETHERNET3",
"ISO3",
"HSCAN2",
"HSCAN3",
"OP_ETHERNET4",
"OP_ETHERNET5",
"ISO4",
"LIN2",
"LIN3",
"LIN4",
"MOST",
"RED_APP_ERROR",
"CGI",
"3G_RESET_STATUS",
"3G_FB_STATUS",
"3G_APP_SIGNAL_STATU",
"3G_READ_DATALINK_CM_TX_MSG",
"3G_READ_DATALINK_CM_RX_MSG",
"3G_LOGGING_OVERFLOW",
"3G_READ_SETTINGS_EX",
"HSCAN4",
"HSCAN5",
"RS232",
"UART",
"UART2",
"UART3",
"UART4",
"SWCAN2",
"ETHERNET_DAQ",
"DATA_TO_HOST",
"TEXTAPI_TO_HOST",
"I2C1",
"SPI1",
"OP_ETHERNET6",
"RED_VBAT",
"OP_ETHERNET7",
"OP_ETHERNET8",
"OP_ETHERNET9",
"OP_ETHERNET10",
"OP_ETHERNET11",
"FLEXRAY1A",
"FLEXRAY1B",
"FLEXRAY2A",
"FLEXRAY2B",
"LIN5",
"FLEXRAY",
"FLEXRAY2",
"OP_ETHERNET12",
"MOST25",
"MOST50",
"MOST150",
"ETHERNET",
"GMFSA",
"TCP",
"HSCAN6",
"HSCAN7",
"LIN6",
"LSFTCAN2" };

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
    std::multimap<uint64_t, std::vector<unsigned char>> messageCache;
    Kompex::SQLiteDatabase* m_pDb;

    void FlushCache(size_t numToFlush = 0);
    void SaveMessage(Kompex::SQLiteStatement& insertMessage, uint64_t timestamp, const std::vector<unsigned char>& data);

public:
    VSBInfo(Kompex::SQLiteDatabase* pDb);
    ~VSBInfo();

    void ProcessMessage(const std::vector<unsigned char>& data);

    void SaveInfo() const
    {
        for (std::map<int, NetworkInfo>::const_iterator it = m_mapNetworks.begin(); it != m_mapNetworks.end(); ++it)
        {
            (*it).second.SaveInfo(m_pDb);
        }
    }
};

bool CreateDb(const char* pVsbPath, const char* pDbPath, ProgressFunc prog);
bool WriteVsb(const char* pDbPath, const char* pVsbPath, const char* pFilter, ProgressFunc prog);
