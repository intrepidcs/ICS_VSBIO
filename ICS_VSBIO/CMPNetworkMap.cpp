// CMPNetworkMap.cpp
// Hand-editable table mapping CMP DeviceId + InterfaceId to VSB NetworkID/NetworkID2.
//
// How to use:
// - Paste rows using the MAP macro: MAP(<DeviceId>, <InterfaceId>, <NetworkID>, <NetworkID2>)
//   Example: MAP(0x1234, 0x07000000, NETID_HSCAN7, 0)
// - You can also use decimal values for NetworkID/NetworkID2.
// - Keep entries unique by (deviceId, interfaceId).

#include "CMPNetworkMap.h"
#include "VSBIO/VSBFlags.h" // For NETID_* constants
#include "pugixml.hpp"

#include <cstdlib>
#include <string>

#include <cstring>

#include <cstddef>
#include <unordered_map>

// Optional runtime override of the CMP network map loaded from a VSDB file.
static std::unordered_map<uint64_t, uint16_t> gOverrideMap;
static bool gOverrideMapActive = false;

// Parse a single <Entry> element with child text elements for CmpDeviceID, CmpInterfaceID, VspyNetworkID.
static inline bool ParseCmpEntry(const pugi::xml_node& entry, CmpNetworkMapEntry& out)
{
    pugi::xml_node devNode = entry.child("CmpDeviceID");
    pugi::xml_node ifNode  = entry.child("CmpInterfaceID");
    pugi::xml_node netNode = entry.child("VspyNetworkID");

    // Fallback to legacy element names
    if (!devNode) devNode = entry.child("DeviceId");
    if (!ifNode)  ifNode  = entry.child("InterfaceId");
    if (!netNode) netNode = entry.child("NetworkId16");

    if (!devNode || !ifNode) return false;

    unsigned long deviceId    = strtoul(devNode.text().get(), nullptr, 10);
    unsigned long interfaceId = strtoul(ifNode.text().get(), nullptr, 10);
    unsigned long vspyNetId   = 0;

    if (netNode)
    {
        vspyNetId = strtoul(netNode.text().get(), nullptr, 10);
    }
    else
    {
        // Legacy split bytes (NetworkId/NetworkId2)
        pugi::xml_node n0Node = entry.child("NetworkId");
        pugi::xml_node n1Node = entry.child("NetworkId2");
        if (!n0Node || !n1Node) return false;
        unsigned long n0 = strtoul(n0Node.text().get(), nullptr, 10);
        unsigned long n1 = strtoul(n1Node.text().get(), nullptr, 10);
        if (n0 > 0xFF || n1 > 0xFF) return false;
        vspyNetId = (n1 << 8) | n0;
    }

    if (deviceId > 0xFFFF || vspyNetId > 0xFFFF) return false;

    out.deviceId    = static_cast<uint16_t>(deviceId);
    out.interfaceId = static_cast<uint32_t>(interfaceId);
    out.networkId16 = static_cast<uint16_t>(vspyNetId);
    return true;
}

// Recursively find a node named "CmpNetworkMap" or "CmpNetwrokMap" (legacy typo).
static pugi::xml_node FindCmpNetworkMapNode(const pugi::xml_node& node)
{
    if (strcmp(node.name(), "CmpNetworkMap") == 0 || strcmp(node.name(), "CmpNetwrokMap") == 0)
        return node;
    for (pugi::xml_node child = node.first_child(); child; child = child.next_sibling())
    {
        pugi::xml_node found = FindCmpNetworkMapNode(child);
        if (found) return found;
    }
    return pugi::xml_node();
}

// Macro to make row definition simple and spreadsheet-friendly.
// Define mapping macros. Prefer MAP16(DEV, IFACE, NETID16). For spreadsheets with separate bytes,
// use MAP2(DEV, IFACE, NID, NID2) which composes the 16-bit value.
#define MAP16(DEV, IFACE, NID16) { static_cast<uint16_t>(DEV), static_cast<uint32_t>(IFACE), static_cast<uint16_t>(NID16) }
#define MAP2(DEV, IFACE, NID, NID2) { static_cast<uint16_t>(DEV), static_cast<uint32_t>(IFACE), static_cast<uint16_t>( ((static_cast<uint16_t>(NID2) << 8) | static_cast<uint16_t>(NID)) ) }

// Hand-editable table: add your mappings here.
// TIP: If pasting from CSV "deviceId,interfaceId,networkId,networkId2", do a quick replace to wrap with MAP(...).
static const CmpNetworkMapEntry kCmpNetworkMap[] = {
    // CMP Device ID, CMP Interface ID, VSB NetworkID16 (NetworkID2 << 8 | NetworkID)         
    MAP16(0x0001, CMP_NET_ID_OP_ETHERNET1, NETID_OP_ETHERNET1), 
    MAP16(0x0001, CMP_NET_ID_OP_ETHERNET2, NETID_OP_ETHERNET2), 
    MAP16(0x0001, CMP_NET_ID_OP_ETHERNET3, NETID_OP_ETHERNET3), 
    MAP16(0x0001, CMP_NET_ID_OP_ETHERNET4, NETID_OP_ETHERNET4), 
    MAP16(0x0001, CMP_NET_ID_OP_ETHERNET5, NETID_OP_ETHERNET5), 
    MAP16(0x0001, CMP_NET_ID_OP_ETHERNET6, NETID_OP_ETHERNET6), 
    MAP16(0x0001, CMP_NET_ID_OP_ETHERNET7, NETID_OP_ETHERNET7), 
    MAP16(0x0001, CMP_NET_ID_OP_ETHERNET8, NETID_OP_ETHERNET8), 

    MAP16(0x0002, CMP_NET_ID_OP_ETHERNET1, NETID_OP_ETHERNET9), 
    MAP16(0x0002, CMP_NET_ID_OP_ETHERNET2, NETID_OP_ETHERNET10), 
    MAP16(0x0002, CMP_NET_ID_OP_ETHERNET3, NETID_OP_ETHERNET11), 
    MAP16(0x0002, CMP_NET_ID_OP_ETHERNET4, NETID_OP_ETHERNET12), 
    MAP16(0x0002, CMP_NET_ID_OP_ETHERNET5, NETID_OP_ETHERNET13), 
    MAP16(0x0002, CMP_NET_ID_OP_ETHERNET6, NETID_OP_ETHERNET14), 
    MAP16(0x0002, CMP_NET_ID_OP_ETHERNET7, NETID_OP_ETHERNET15), 
    MAP16(0x0002, CMP_NET_ID_OP_ETHERNET8, NETID_OP_ETHERNET16), 

    MAP16(0x0002, CMP_NET_ID_LIN, NETID_LIN_17),
    MAP16(0x0002, CMP_NET_ID_LIN2, NETID_LIN_18),
    MAP16(0x0002, CMP_NET_ID_LIN3, NETID_LIN_19),
    MAP16(0x0002, CMP_NET_ID_LIN4, NETID_LIN_20),
    MAP16(0x0002, CMP_NET_ID_LIN5, NETID_LIN_21),
    MAP16(0x0002, CMP_NET_ID_LIN6, NETID_LIN_22),
    MAP16(0x0002, CMP_NET_ID_LIN_07, NETID_LIN_23),
    MAP16(0x0002, CMP_NET_ID_LIN_08, NETID_LIN_24),
    MAP16(0x0002, CMP_NET_ID_LIN_09, NETID_LIN_25),
    MAP16(0x0002, CMP_NET_ID_LIN_10, NETID_LIN_26),
    MAP16(0x0002, CMP_NET_ID_LIN_11, NETID_LIN_27),
    MAP16(0x0002, CMP_NET_ID_LIN_12, NETID_LIN_28),
    MAP16(0x0002, CMP_NET_ID_LIN_13, NETID_LIN_29),
    MAP16(0x0002, CMP_NET_ID_LIN_14, NETID_LIN_30),
    MAP16(0x0002, CMP_NET_ID_LIN_15, NETID_LIN_31),
    MAP16(0x0002, CMP_NET_ID_LIN_16, NETID_LIN_32),

    MAP16(0x0003, CMP_NET_ID_LIN, NETID_LIN_33),
    MAP16(0x0003, CMP_NET_ID_LIN2, NETID_LIN_34),
    MAP16(0x0003, CMP_NET_ID_LIN3, NETID_LIN_35),
    MAP16(0x0003, CMP_NET_ID_LIN4, NETID_LIN_36),
    MAP16(0x0003, CMP_NET_ID_LIN5, NETID_LIN_37),
    MAP16(0x0003, CMP_NET_ID_LIN6, NETID_LIN_38),
    MAP16(0x0003, CMP_NET_ID_LIN_07, NETID_LIN_39),
    MAP16(0x0003, CMP_NET_ID_LIN_08, NETID_LIN_40),
    MAP16(0x0003, CMP_NET_ID_LIN_09, NETID_LIN_41),
    MAP16(0x0003, CMP_NET_ID_LIN_10, NETID_LIN_42),
    MAP16(0x0003, CMP_NET_ID_LIN_11, NETID_LIN_43),
    MAP16(0x0003, CMP_NET_ID_LIN_12, NETID_LIN_44),
    MAP16(0x0003, CMP_NET_ID_LIN_13, NETID_LIN_45),
    MAP16(0x0003, CMP_NET_ID_LIN_14, NETID_LIN_46),
    MAP16(0x0003, CMP_NET_ID_LIN_15, NETID_LIN_47),
    MAP16(0x0003, CMP_NET_ID_LIN_16, NETID_LIN_48),

    MAP16(0x0001, CMP_NET_ID_HSCAN, NETID_DWCAN_17),
    MAP16(0x0001, CMP_NET_ID_HSCAN2, NETID_DWCAN_18),
    MAP16(0x0001, CMP_NET_ID_HSCAN3, NETID_DWCAN_19),
    MAP16(0x0001, CMP_NET_ID_HSCAN4, NETID_DWCAN_20),
    MAP16(0x0001, CMP_NET_ID_HSCAN5, NETID_DWCAN_21),
    MAP16(0x0001, CMP_NET_ID_HSCAN6, NETID_DWCAN_22),
    MAP16(0x0001, CMP_NET_ID_HSCAN7, NETID_DWCAN_23),
    MAP16(0x0001, CMP_NET_ID_MSCAN, NETID_DWCAN_24),
    MAP16(0x0001, CMP_NET_ID_DWCAN_09, NETID_DWCAN_25),
    MAP16(0x0001, CMP_NET_ID_DWCAN_10, NETID_DWCAN_26),
    MAP16(0x0001, CMP_NET_ID_DWCAN_11, NETID_DWCAN_27),
    MAP16(0x0001, CMP_NET_ID_DWCAN_12, NETID_DWCAN_28),
    MAP16(0x0001, CMP_NET_ID_DWCAN_13, NETID_DWCAN_29),
    MAP16(0x0001, CMP_NET_ID_DWCAN_14, NETID_DWCAN_30),
    MAP16(0x0001, CMP_NET_ID_DWCAN_15, NETID_DWCAN_31),
    MAP16(0x0001, CMP_NET_ID_DWCAN_16, NETID_DWCAN_32),

    MAP16(0x0002, CMP_NET_ID_HSCAN, NETID_DWCAN_33),
    MAP16(0x0003, CMP_NET_ID_HSCAN, NETID_DWCAN_34),

    // Sentinel to avoid zero-length array; will never match typical IDs.
    MAP16(0xFFFF, 0xFFFFFFFFu, 0xFFFF)
};

static constexpr size_t kCmpNetworkMapSize = sizeof(kCmpNetworkMap) / sizeof(CmpNetworkMapEntry);

// Helper: combine deviceId and interfaceId into a single 64-bit key for hashing
static inline uint64_t MakeMapKey(uint16_t deviceId, uint32_t interfaceId)
{
    return (static_cast<uint64_t>(deviceId) << 32) | static_cast<uint64_t>(interfaceId);
}

// Build hash map once for O(1) lookups
static const std::unordered_map<uint64_t, uint16_t>& GetCmpNetworkHashMap()
{
    static std::unordered_map<uint64_t, uint16_t> hashMap;
    static bool initialized = false;
    
    if (!initialized) {
        hashMap.reserve(kCmpNetworkMapSize);
        for (size_t i = 0; i < kCmpNetworkMapSize; ++i) {
            const auto& e = kCmpNetworkMap[i];
            uint64_t key = MakeMapKey(e.deviceId, e.interfaceId);
            hashMap[key] = e.networkId16;
        }
        initialized = true;
    }
    
    return hashMap;
}

bool LoadCmpNetworkMapFromVsdb(const char* vsdbFilePath)
{
    gOverrideMap.clear();
    gOverrideMapActive = false;

    if (!vsdbFilePath) return false;

    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(vsdbFilePath);
    if (!result) return false;

    pugi::xml_node mapNode = FindCmpNetworkMapNode(doc);
    if (!mapNode) return false;

    std::unordered_map<uint64_t, uint16_t> loaded;
    for (pugi::xml_node entry = mapNode.child("Entry"); entry; entry = entry.next_sibling("Entry"))
    {
        CmpNetworkMapEntry parsed{};
        if (!ParseCmpEntry(entry, parsed))
            continue;
        const uint64_t key = (static_cast<uint64_t>(parsed.deviceId) << 32) | static_cast<uint64_t>(parsed.interfaceId);
        loaded[key] = parsed.networkId16;
    }

    if (loaded.empty()) return false;

    gOverrideMap.swap(loaded);
    gOverrideMapActive = true;
    return true;
}

bool CmpMapLookup16(uint16_t deviceId, uint32_t interfaceId, uint16_t& outNetId16)
{
    uint64_t key = MakeMapKey(deviceId, interfaceId);
        if (gOverrideMapActive)
        {
            auto itOverride = gOverrideMap.find(key);
            if (itOverride != gOverrideMap.end()) {
                outNetId16 = itOverride->second;
                return true;
            }
        }

        const auto& hashMap = GetCmpNetworkHashMap();
        auto it = hashMap.find(key);
        if (it != hashMap.end()) {
            outNetId16 = it->second;
            return true;
        }
    return false;
}
