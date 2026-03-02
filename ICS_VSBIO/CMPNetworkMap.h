// CMPNetworkMap.h
// Hand-editable mapping from CMP DeviceId + InterfaceId to VSB NetworkID + NetworkID2.
#pragma once

#include <cstdint>
#include "VSBIO/VSBStruct.h"

// A single mapping entry: uniquely maps a CMP device/interface pair to a 16-bit VSB NetworkID.
// The 16-bit form encodes NetworkID/NetworkID2 as (NetworkID2 << 8) | NetworkID.
struct CmpNetworkMapEntry {
    uint16_t deviceId;      // From CMP header (BE16 at cmpOffset+2)
    uint32_t interfaceId;   // From CMP data header (LE32 at dataHdrOffset+8)
    uint16_t networkId16;   // Combined 16-bit NetworkID
};

/*CMP InterfaceIds - constants designed to match ReadLE32 output from wire format*/
#define CMP_NET_ID_NOT_APPLICABLE 0x00
/*CAN IDs - 0x01- 0xFF*/
#define CMP_NET_ID_HSCAN	0x01000000
#define CMP_NET_ID_HSCAN2	0x02000000
#define CMP_NET_ID_HSCAN3	0x03000000
#define CMP_NET_ID_HSCAN4	0x04000000
#define CMP_NET_ID_HSCAN5	0x05000000
#define CMP_NET_ID_HSCAN6	0x06000000
#define CMP_NET_ID_HSCAN7	0x07000000
#define CMP_NET_ID_MSCAN	0x08000000
#define CMP_NET_ID_DWCAN_09 0x09000000
#define CMP_NET_ID_DWCAN_10 0x0A000000
#define CMP_NET_ID_DWCAN_11 0x0B000000
#define CMP_NET_ID_DWCAN_12 0x0C000000
#define CMP_NET_ID_DWCAN_13 0x0D000000
#define CMP_NET_ID_DWCAN_14 0x0E000000
#define CMP_NET_ID_DWCAN_15 0x0F000000
#define CMP_NET_ID_DWCAN_16 0x10000000
#define CMP_NET_ID_LSFTCAN	0x11000000
#define CMP_NET_ID_LSFTCAN2 0x12000000
#define CMP_NET_ID_SWCAN	0x13000000
#define CMP_NET_ID_SWCAN2	0x14000000
/*LIN IDs - 0x0101- 0x01FF*/
#define CMP_NET_ID_LIN	  0x01010000
#define CMP_NET_ID_LIN2	  0x02010000
#define CMP_NET_ID_LIN3	  0x03010000
#define CMP_NET_ID_LIN4	  0x04010000
#define CMP_NET_ID_LIN5	  0x05010000
#define CMP_NET_ID_LIN6	  0x06010000
#define CMP_NET_ID_LIN_07 0x07010000
#define CMP_NET_ID_LIN_08 0x08010000
#define CMP_NET_ID_LIN_09 0x09010000
#define CMP_NET_ID_LIN_10 0x0A010000
#define CMP_NET_ID_LIN_11 0x0B010000
#define CMP_NET_ID_LIN_12 0x0C010000
#define CMP_NET_ID_LIN_13 0x0D010000
#define CMP_NET_ID_LIN_14 0x0E010000
#define CMP_NET_ID_LIN_15 0x0F010000
#define CMP_NET_ID_LIN_16 0x10010000
/*FlexRay IDs - 0x0201- 0x02FF*/
#define CMP_NET_ID_FLEXRAY	 0x01020000
#define CMP_NET_ID_FLEXRAY1A 0x02020000
#define CMP_NET_ID_FLEXRAY1B 0x03020000
#define CMP_NET_ID_FLEXRAY2	 0x04020000
#define CMP_NET_ID_FLEXRAY2A 0x05020000
#define CMP_NET_ID_FLEXRAY2B 0x06020000
/*Ethernet IDs - 0x0301- 0x03FF*/
#define CMP_NET_ID_ETHERNET	 0x01030000
#define CMP_NET_ID_ETHERNET2 0x02030000
#define CMP_NET_ID_ETHERNET3 0x03030000
/*GPIO IDs - 0x0401- 0x04FF*/
#define CMP_NET_ID_GPIO_01 0x01040000
#define CMP_NET_ID_GPIO_02 0x02040000
#define CMP_NET_ID_GPIO_03 0x03040000
#define CMP_NET_ID_GPIO_04 0x04040000
#define CMP_NET_ID_GPIO_05 0x05040000
#define CMP_NET_ID_GPIO_06 0x06040000
#define CMP_NET_ID_GPIO_07 0x07040000
#define CMP_NET_ID_GPIO_08 0x08040000
/*Serial IDs - 0x0501- 0x05FF*/
#define CMP_NET_ID_UART	 0x01050000
#define CMP_NET_ID_UART2 0x02050000
#define CMP_NET_ID_UART3 0x03050000
#define CMP_NET_ID_UART4 0x04050000
#define CMP_NET_ID_RS232 0x05050000
/*SPI IDs - 0x0601- 0x06FF*/
#define CMP_NET_ID_SPI1 0x01060000
#define CMP_NET_ID_SPI2 0x02060000
/*DIO IDs - 0x0701- 0x07FF*/
#define CMP_NET_ID_I2C1 0x01070000
#define CMP_NET_ID_I2C2 0x02070000
#define CMP_NET_ID_I2C3 0x03070000
#define CMP_NET_ID_I2C4 0x04070000
/*OP Eternet IDs - 0x0801- 0x08FF*/
#define CMP_NET_ID_OP_ETHERNET1	 0x01080000
#define CMP_NET_ID_OP_ETHERNET2	 0x02080000
#define CMP_NET_ID_OP_ETHERNET3	 0x03080000
#define CMP_NET_ID_OP_ETHERNET4	 0x04080000
#define CMP_NET_ID_OP_ETHERNET5	 0x05080000
#define CMP_NET_ID_OP_ETHERNET6	 0x06080000
#define CMP_NET_ID_OP_ETHERNET7	 0x07080000
#define CMP_NET_ID_OP_ETHERNET8	 0x08080000
#define CMP_NET_ID_OP_ETHERNET9	 0x09080000
#define CMP_NET_ID_OP_ETHERNET10 0x0A080000
#define CMP_NET_ID_OP_ETHERNET11 0x0B080000
#define CMP_NET_ID_OP_ETHERNET12 0x0C080000
#define CMP_NET_ID_OP_ETHERNET13 0x0D080000
#define CMP_NET_ID_OP_ETHERNET14 0x0E080000
#define CMP_NET_ID_OP_ETHERNET15 0x0F080000
#define CMP_NET_ID_OP_ETHERNET16 0x10080000
/*A2B IDs - 0x0901- 0x09FF*/
#define CMP_NET_ID_A2B1 0x01090000
#define CMP_NET_ID_A2B2 0x02090000


// Lookup: returns true and fills outNetId16 when found.
bool CmpMapLookup16(uint16_t deviceId, uint32_t interfaceId, uint16_t& outNetId16);

// Optional: load CMP network map overrides from a VSDB file containing a <CmpNetworkMap> section.
// Returns true when a map was found and loaded; falls back to the built-in map otherwise.
#if defined(_WINDLL) && defined(VSBIODLL_EXPORTS)
  #define CMPMAP_API __declspec(dllexport)
#elif defined(_WINDLL)
  #define CMPMAP_API __declspec(dllimport)
#else
  #define CMPMAP_API
#endif
CMPMAP_API bool LoadCmpNetworkMapFromVsdb(const char* vsdbFilePath);

/**
 * UnwrapCMPToNative - Main CMP unwrapping function
 * 
 * Process flow:
 * 1. Read VSB messages sequentially from input file
 * 2. Detect Ethernet messages with CMP ethertype (0x99FE)
 * 3. Validate CMP version (0x01) and message type (CAP_DATA_MSG = 0x01)
 * 4. Dispatch to protocol-specific unwrapper based on payload type:
 *    - 0x01: CAN (TryUnwrapCmpCan)
 *    - 0x02: CAN FD (TryUnwrapCmpCanFd) 
 *    - 0x03: LIN (TryUnwrapCmpLin)
 *    - 0x08: Ethernet (TryUnwrapCmpEthernet - handles segmentation)
 * 5. Map CMP DeviceId+InterfaceId to native VSB Network IDs via CMPNetworkMap
 * 6. Write unwrapped messages with proper protocol and network ID to output
 * 7. Pass through non-CMP messages unchanged
 * 
 * @param inputFilePath - Full path to input VSB file with CMP Ethernet frames
 * @param outputFilePath - Full path to output VSB file with native messages
 * @param sortOutput - 0=Sort frames by timestamp (default), 1=No sorting (preserve input order)
 * @param timestampSource - 0=CMP timestamp, 1=Ethernet timestamp (recommended)
 * @param prog - Optional progress callback (NULL or function returning bool)
 * @return true on success, false on I/O error
 */
bool UnwrapCMPToNative(const char* inputFilePath, const char* outputFilePath, int sortOutput, int timestampSource, ProgressFunc prog, int outputInfoToTxt);
