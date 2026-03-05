#include "CMPNetworkMap.h"
#include "VSBIODLL.h"
#include "VSBIO/VSBIO.h"
#include "VSBIO/VSBFlags.h"
#include "VSBIO/MessageTimeDecoderVSB.h"
#include "VSBIO/OFile.h"
#include <vector>
#include <map>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iomanip>

static inline unsigned short ReadBE16(const unsigned char* p)
{
	return (unsigned short)((p[0] << 8) | p[1]);
}

static inline uint32_t ReadBE32(const unsigned char* p)
{
	return (uint32_t)((p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]);
}

static inline uint16_t ReadLE16(const unsigned char* p)
{
	return (uint16_t)(p[0] | (p[1] << 8));
}

static inline uint32_t ReadLE32(const unsigned char* p)
{
	return (uint32_t)(p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24));
}

static inline uint64_t ReadLE64(const unsigned char* p)
{
	return (uint64_t)((uint64_t)p[0] | ((uint64_t)p[1] << 8) | ((uint64_t)p[2] << 16) | ((uint64_t)p[3] << 24)
		| ((uint64_t)p[4] << 32) | ((uint64_t)p[5] << 40) | ((uint64_t)p[6] << 48) | ((uint64_t)p[7] << 56));
}

static inline uint64_t ReadBE64(const unsigned char* p)
{
	return (uint64_t)(((uint64_t)p[0] << 56) | ((uint64_t)p[1] << 48) | ((uint64_t)p[2] << 40) | ((uint64_t)p[3] << 32)
		| ((uint64_t)p[4] << 24) | ((uint64_t)p[5] << 16) | ((uint64_t)p[6] << 8) | (uint64_t)p[7]);
}

static inline void CopyTimestampFromOuter(const icsSpyMessageVSB& outer, icsSpyMessageVSB& out)
{
	out.TimeHardware = outer.TimeHardware;
	out.TimeHardware2 = outer.TimeHardware2;
	out.TimeSystem = outer.TimeSystem;
	out.TimeSystem2 = outer.TimeSystem2;
	out.TimeStampHardwareID = outer.TimeStampHardwareID;
	out.TimeStampSystemID = outer.TimeStampSystemID;
}

static inline void SetNetworkID16(icsSpyMessageVSB& msg, const unsigned short netid16)
{
	msg.NetworkID = (unsigned char)(netid16 & 0xFF);
	msg.NetworkID2 = (unsigned char)((netid16 >> 8) & 0xFF);
}

// Generic CMP→VSB network mapping: try DeviceId+InterfaceId mapping table,
// then fall back to a provided 16-bit NetworkID when available.
// Returns true if a valid network mapping was found or fallback was applied; false otherwise.
static inline bool SetCmpNetworkFromMap(icsSpyMessageVSB& msg, const uint16_t cmpDeviceId, const uint32_t cmpInterfaceId, const unsigned short fallbackNetId)
{
	uint16_t netid16 = 0;
	if (CmpMapLookup16(cmpDeviceId, cmpInterfaceId, netid16)) {
		SetNetworkID16(msg, netid16);
		return true;
	}
	if (fallbackNetId != NETID_INVALID) {
		SetNetworkID16(msg, fallbackNetId);
		return true;
	}
	return false;
}

// CMP constants derived from coremini_lib definitions
static constexpr size_t   CMP_HEADER_OFFSET                    = 14;
static constexpr size_t   CMP_HEADER_SIZE                      = 8;
static constexpr size_t   CMP_DATA_MESSAGE_HEADER_SIZE         = 16;
static constexpr uint8_t  CMP_DATA_MESSAGE_PAYLOAD_TYPE_CAN      = 0x01;
static constexpr uint8_t  CMP_DATA_MESSAGE_PAYLOAD_TYPE_CANFD    = 0x02;
static constexpr uint8_t  CMP_DATA_MESSAGE_PAYLOAD_TYPE_LIN      = 0x03;
static constexpr uint8_t  CMP_DATA_MESSAGE_PAYLOAD_TYPE_ETHERNET = 0x08;

// SEG field values (bits 6-7 of Common Flags)
static constexpr uint8_t  CMP_SEG_UNSEGMENTED  = 0x00;
static constexpr uint8_t  CMP_SEG_START        = 0x40; // 0b01000000
static constexpr uint8_t  CMP_SEG_CONTINUATION = 0x80; // 0b10000000
static constexpr uint8_t  CMP_SEG_END          = 0xC0; // 0b11000000
static constexpr uint8_t  CMP_SEG_MASK         = 0xC0;

// CMP protocol constants
static constexpr uint16_t CMP_ETHERTYPE            = 0x99FE;
static constexpr uint8_t  CMP_VERSION              = 0x01;
static constexpr uint8_t  CMP_MESSAGE_TYPE_CAP_DATA = 0x01;
static constexpr uint8_t  CMP_COMMON_FLAG_DIR_ON_IF = 0x10;

// Ethernet frame offsets
static constexpr size_t   ETH_ETHERTYPE_OFFSET = 12;

// CMP header field offsets (relative to CMP header start)
static constexpr size_t   CMP_HDR_VERSION_OFFSET    = 0;
static constexpr size_t   CMP_HDR_DEVICE_ID_OFFSET  = 2;
static constexpr size_t   CMP_HDR_MSG_TYPE_OFFSET   = 4;
static constexpr size_t   CMP_HDR_STREAM_ID_OFFSET  = 5;
static constexpr size_t   CMP_HDR_SEQ_COUNTER_OFFSET = 6;

// CMP data message header field offsets (relative to data header start)
static constexpr size_t   CMP_DATA_HDR_TIMESTAMP_OFFSET    = 0;
static constexpr size_t   CMP_DATA_HDR_INTERFACE_ID_OFFSET = 8;
static constexpr size_t   CMP_DATA_HDR_COMMON_FLAGS_OFFSET = 12;
static constexpr size_t   CMP_DATA_HDR_PAYLOAD_TYPE_OFFSET = 13;
static constexpr size_t   CMP_DATA_HDR_PAYLOAD_LEN_OFFSET  = 14;

// CAN/CANFD payload header offsets and size (relative to payload header start)
static constexpr size_t   CMP_CAN_HDR_FLAGS_OFFSET    = 0;
static constexpr size_t   CMP_CAN_HDR_ID_OFFSET       = 4;
static constexpr size_t   CMP_CAN_HDR_DLC_OFFSET      = 14;
static constexpr size_t   CMP_CAN_HDR_DATA_LEN_OFFSET = 15;
static constexpr size_t   CMP_CAN_HDR_SIZE            = 16;

// CANFD-specific payload header offsets
static constexpr size_t   CMP_CANFD_HDR_RESERVED_OFFSET = 2;
static constexpr size_t   CMP_CANFD_HDR_ERR_POS_OFFSET  = 12;

// LIN payload header offsets and size (relative to payload header start)
static constexpr size_t   CMP_LIN_HDR_FLAGS_OFFSET    = 0;
static constexpr size_t   CMP_LIN_HDR_PID_OFFSET      = 4;
static constexpr size_t   CMP_LIN_HDR_CRC_OFFSET      = 6;
static constexpr size_t   CMP_LIN_HDR_DATA_LEN_OFFSET = 7;
static constexpr size_t   CMP_LIN_HDR_SIZE            = 8;

// Ethernet payload header offsets and minimum size (relative to payload start)
static constexpr size_t   CMP_ETH_HDR_FLAGS_OFFSET    = 0;
static constexpr size_t   CMP_ETH_HDR_DATA_LEN_OFFSET = 4;
static constexpr size_t   CMP_ETH_HDR_DATA_OFFSET     = 6;
static constexpr size_t   CMP_ETH_HDR_MIN_SIZE        = 6;

// Heuristic stub: we currently only detect CMP ethertype and version; detailed LIN payload parsing will follow.
static bool TryUnwrapCmpLin(const icsSpyMessageVSB& inEth, const unsigned char* edp, const size_t edpLen,
	const int timestampSource, icsSpyMessageVSB& outLin)
{
	const unsigned short ethertype = ReadBE16(edp + ETH_ETHERTYPE_OFFSET);
	if (ethertype != CMP_ETHERTYPE) return false;

	auto parseAtOffset = [&](size_t cmpOffset) -> bool {
		if (edpLen < (cmpOffset + CMP_HEADER_SIZE + CMP_DATA_MESSAGE_HEADER_SIZE)) return false;

		const unsigned char cmpVersion = edp[cmpOffset + CMP_HDR_VERSION_OFFSET];
		if (cmpVersion != CMP_VERSION) return false;

		const size_t dataHdrOffset = cmpOffset + CMP_HEADER_SIZE;
		const uint16_t cmpDeviceId = ReadBE16(edp + cmpOffset + CMP_HDR_DEVICE_ID_OFFSET);
		const uint32_t cmpInterfaceId = ReadLE32(edp + dataHdrOffset + CMP_DATA_HDR_INTERFACE_ID_OFFSET);
		const unsigned char commonFlags = edp[dataHdrOffset + CMP_DATA_HDR_COMMON_FLAGS_OFFSET];
		const unsigned char payloadType = edp[dataHdrOffset + CMP_DATA_HDR_PAYLOAD_TYPE_OFFSET];
		if (payloadType != CMP_DATA_MESSAGE_PAYLOAD_TYPE_LIN) return false;

		const size_t linHdrOffset = dataHdrOffset + CMP_DATA_MESSAGE_HEADER_SIZE;
		if (edpLen < (linHdrOffset + CMP_LIN_HDR_SIZE)) return false;

		const uint16_t linFlags = ReadLE16(edp + linHdrOffset + CMP_LIN_HDR_FLAGS_OFFSET);
		const unsigned char pid = edp[linHdrOffset + CMP_LIN_HDR_PID_OFFSET];
		const unsigned char crc = edp[linHdrOffset + CMP_LIN_HDR_CRC_OFFSET];
		const unsigned char dataLen = edp[linHdrOffset + CMP_LIN_HDR_DATA_LEN_OFFSET];

		const size_t linDataOffset = linHdrOffset + CMP_LIN_HDR_SIZE;
		if (edpLen < (linDataOffset + (size_t)dataLen)) return false;

		// Prepare output LIN VSB message
		memset(&outLin, 0, sizeof(outLin));
		outLin.Protocol = SPY_PROTOCOL_LIN;
		outLin.NodeID = 0;
		// Per Golden VSB: LIN uses 1-byte header
		outLin.NumberBytesHeader = 1;
		// LIN payload fits in Data[8]; no extra payload
		outLin.ExtraDataPtrEnabled = 0;
		outLin.ExtraDataPtr = 0;
		outLin.MessagePieceID = 0;
		// Golden shows DescriptionID = 1 for LIN
		outLin.DescriptionID = 1;
		outLin.MiscData = 0;

		// Mark as network message type
		outLin.StatusBitField |= SPY_STATUS_NETWORK_MESSAGE_TYPE;

		// Set network using CmpMapLookup16 exclusively
		if (!SetCmpNetworkFromMap(outLin, cmpDeviceId, cmpInterfaceId, NETID_INVALID)) return false;

		// Store LIN ID (6-bit, no parity) in first header byte
		const unsigned char linId = (unsigned char)(pid & 0x3F);
		unsigned char* hdrBytes = reinterpret_cast<unsigned char*>(&outLin.ArbIDOrHeader);
		hdrBytes[0] = linId;

		// Historical VSPY behavior: store up to first 2 data bytes in header after ID
		const unsigned char headerDataBytes = (dataLen < 2) ? dataLen : 2;
		if (headerDataBytes > 0)
		{
			for (unsigned int i = 0; i < headerDataBytes; ++i)
				hdrBytes[1 + i] = edp[linDataOffset + i];
			outLin.NumberBytesHeader = (unsigned char)(outLin.NumberBytesHeader + headerDataBytes);
		}

		// Copy data bytes exactly as indicated by dataLen (CRC not included)
		if (dataLen == 0)
		{
			outLin.NumberBytesData = 0;
			outLin.StatusBitField2 |= SPY_STATUS2_LIN_NO_SLAVE_DATA;
		}
		else
		{
			// Copy remaining payload after the bytes stored in header
			const unsigned int remaining = (unsigned int)dataLen - headerDataBytes;
			for (unsigned int i = 0; i < remaining && i < 8; ++i)
				outLin.Data.data[i] = edp[linDataOffset + headerDataBytes + i];
			outLin.NumberBytesData = (unsigned char)remaining;
		}

		// Basic flag mappings
		const bool parityErr = (linFlags & (1u << 10)) != 0;
		const bool checksumErr = (linFlags & (1u << 8)) != 0;
		if (parityErr) outLin.StatusBitField2 |= SPY_STATUS2_ISO_PARITY_ERROR;
		if (checksumErr) outLin.StatusBitField |= SPY_STATUS_CHECKSUM_ERROR;

		// Direction: if dirOnIf flag set in CMP commonFlags, mark as TX
		if ((commonFlags & CMP_COMMON_FLAG_DIR_ON_IF) != 0)
			outLin.StatusBitField |= SPY_STATUS_TX_MSG;

		// Place checksum in AckBytes[0] per BLF import convention
		outLin.AckBytes.data[0] = crc;

		// Set timestamp based on timestampSource
		if (timestampSource == 0) {
			// Use CMP timestamp (LIN does not have its own, so use the one from the CMP header)
			const uint64_t cmpTimestamp = ReadBE64(edp + dataHdrOffset + CMP_DATA_HDR_TIMESTAMP_OFFSET);
			CMessageTimeDecoderVSB::SetMessageTime(outLin, cmpTimestamp);
		} else {
			CopyTimestampFromOuter(inEth, outLin);
		}

		return true;
	};

	// Parse at standard CMP offset (after 14-byte Ethernet header)
	return parseAtOffset(CMP_HEADER_OFFSET);
}

// Set CAN network ID using mapping table with fallback to sentinel value
static inline bool SetCmpCanNetwork(icsSpyMessageVSB& msg, const uint16_t cmpDeviceId, const uint32_t cmpIf)
{
	return SetCmpNetworkFromMap(msg, cmpDeviceId, cmpIf, NETID_INVALID);
}

static bool TryUnwrapCmpCan(const icsSpyMessageVSB& inEth, const unsigned char* edp, const size_t edpLen,
	const int timestampSource, icsSpyMessageVSB& outCan, std::vector<unsigned char>& extra)
{
	const unsigned short ethertype = ReadBE16(edp + ETH_ETHERTYPE_OFFSET);
	if (ethertype != CMP_ETHERTYPE) return false;

	auto parseAtOffset = [&](size_t cmpOffset) -> bool {
		if (edpLen < (cmpOffset + CMP_HEADER_SIZE + CMP_DATA_MESSAGE_HEADER_SIZE)) return false;

		const unsigned char cmpVersion = edp[cmpOffset + CMP_HDR_VERSION_OFFSET];
		if (cmpVersion != CMP_VERSION) return false;

		const size_t dataHdrOffset = cmpOffset + CMP_HEADER_SIZE;
		const uint16_t cmpDeviceId = ReadBE16(edp + cmpOffset + CMP_HDR_DEVICE_ID_OFFSET);
		const uint32_t cmpInterfaceId = ReadLE32(edp + dataHdrOffset + CMP_DATA_HDR_INTERFACE_ID_OFFSET);
		const unsigned char commonFlags = edp[dataHdrOffset + CMP_DATA_HDR_COMMON_FLAGS_OFFSET];
		const unsigned char payloadType = edp[dataHdrOffset + CMP_DATA_HDR_PAYLOAD_TYPE_OFFSET];
		if (payloadType != CMP_DATA_MESSAGE_PAYLOAD_TYPE_CAN) return false;

		const size_t canHdrOffset = dataHdrOffset + CMP_DATA_MESSAGE_HEADER_SIZE;
		if (edpLen < (canHdrOffset + CMP_CAN_HDR_SIZE)) return false;

		// const uint16_t canFlags = ReadLE16(edp + canHdrOffset + CMP_CAN_HDR_FLAGS_OFFSET); // Reserved for future use
		// CAN ID word: bytes appear reversed on transmit; interpret as big-endian here
		const uint32_t idWordBE = (uint32_t)((edp[canHdrOffset + CMP_CAN_HDR_ID_OFFSET] << 24) |
			     (edp[canHdrOffset + CMP_CAN_HDR_ID_OFFSET + 1] << 16) |
			     (edp[canHdrOffset + CMP_CAN_HDR_ID_OFFSET + 2] << 8) |
			     (edp[canHdrOffset + CMP_CAN_HDR_ID_OFFSET + 3]));
		// const unsigned char dlc = edp[canHdrOffset + CMP_CAN_HDR_DLC_OFFSET]; // Reserved for future use
		const unsigned char dataLen = edp[canHdrOffset + CMP_CAN_HDR_DATA_LEN_OFFSET];

		// CAN max 8 bytes
		if (dataLen > 8) return false;

		const size_t dataOffset = canHdrOffset + CMP_CAN_HDR_SIZE;
		if (edpLen < (dataOffset + (size_t)dataLen)) return false;

		// Extract CAN ID: lower 29 bits of big-endian ID word
		const uint32_t canId = (idWordBE & 0x1FFFFFFFu);
		// IDE bit is carried in the ID word; read explicitly (MSB)
		const bool ide = ((idWordBE & 0x80000000u) != 0);
		// RTR bit for remote frames
		const bool rtr = ((idWordBE & 0x40000000u) != 0);

		// Prepare output CAN VSB message
		memset(&outCan, 0, sizeof(outCan));
		outCan.Protocol = SPY_PROTOCOL_CAN;
		outCan.NodeID = 0;
		outCan.NumberBytesHeader = 4; // ArbID in header
		outCan.MessagePieceID = 0;
		outCan.DescriptionID = 0;
		outCan.MiscData = 0;

		outCan.StatusBitField |= SPY_STATUS_NETWORK_MESSAGE_TYPE;
		if (ide) {
			outCan.StatusBitField |= SPY_STATUS_XTD_FRAME;
		}
		if (rtr) {
			outCan.StatusBitField |= SPY_STATUS_REMOTE_FRAME;
		}
		// Direction: TX when dirOnIf set
		if ((commonFlags & CMP_COMMON_FLAG_DIR_ON_IF) != 0) {
			outCan.StatusBitField |= SPY_STATUS_TX_MSG;
		}

		// Set network using generic mapping (fallback to sentinel)
		if (!SetCmpCanNetwork(outCan, cmpDeviceId, cmpInterfaceId)) {
			return false; // No valid network mapping, skip unwrapping
		}

		// Header holds ArbID
		outCan.ArbIDOrHeader = canId;

		// CAN stores all data in Data[8], no extra payload
		for (unsigned int i = 0; i < dataLen && i < 8; ++i)
			outCan.Data.data[i] = edp[dataOffset + i];
		outCan.NumberBytesData = dataLen;

		// No extra payload for CAN
		outCan.ExtraDataPtrEnabled = 0;
		outCan.ExtraDataPtr = 0;
		extra.clear();

		// Set timestamp based on timestampSource
		if (timestampSource == 0) {
			const uint64_t cmpTimestamp = ReadBE64(edp + dataHdrOffset + CMP_DATA_HDR_TIMESTAMP_OFFSET);
			CMessageTimeDecoderVSB::SetMessageTime(outCan, cmpTimestamp);
		} else {
			CopyTimestampFromOuter(inEth, outCan);
		}

		return true;
	};

	// Parse at standard CMP offset (after 14-byte Ethernet header)
	return parseAtOffset(CMP_HEADER_OFFSET);
}

static bool TryUnwrapCmpCanFd(const icsSpyMessageVSB& inEth, const unsigned char* edp, const size_t edpLen,
	const int timestampSource, icsSpyMessageVSB& outCan, std::vector<unsigned char>& extra)
{
	// timestampSource is now used below
	const unsigned short ethertype = ReadBE16(edp + ETH_ETHERTYPE_OFFSET);
	if (ethertype != CMP_ETHERTYPE) return false;

	auto parseAtOffset = [&](size_t cmpOffset) -> bool {
		if (edpLen < (cmpOffset + CMP_HEADER_SIZE + CMP_DATA_MESSAGE_HEADER_SIZE)) return false;

		const unsigned char cmpVersion = edp[cmpOffset + CMP_HDR_VERSION_OFFSET];
		if (cmpVersion != CMP_VERSION) return false;

		const size_t dataHdrOffset = cmpOffset + CMP_HEADER_SIZE;
		const uint16_t cmpDeviceId = ReadBE16(edp + cmpOffset + CMP_HDR_DEVICE_ID_OFFSET);
		const uint32_t cmpInterfaceId = ReadLE32(edp + dataHdrOffset + CMP_DATA_HDR_INTERFACE_ID_OFFSET);
		const unsigned char commonFlags = edp[dataHdrOffset + CMP_DATA_HDR_COMMON_FLAGS_OFFSET];
		const unsigned char payloadType = edp[dataHdrOffset + CMP_DATA_HDR_PAYLOAD_TYPE_OFFSET];
		if (payloadType != CMP_DATA_MESSAGE_PAYLOAD_TYPE_CANFD) return false;

		const size_t canHdrOffset = dataHdrOffset + CMP_DATA_MESSAGE_HEADER_SIZE;
		if (edpLen < (canHdrOffset + CMP_CAN_HDR_SIZE)) return false;

		const uint16_t canfdFlags = ReadLE16(edp + canHdrOffset + CMP_CAN_HDR_FLAGS_OFFSET);
		// const uint16_t reserved = ReadLE16(edp + canHdrOffset + CMP_CANFD_HDR_RESERVED_OFFSET);
		// CAN FD ID word: bytes appear reversed on transmit; interpret as big-endian here
		const uint32_t idWordBE = (uint32_t)((edp[canHdrOffset + CMP_CAN_HDR_ID_OFFSET] << 24) |
			     (edp[canHdrOffset + CMP_CAN_HDR_ID_OFFSET + 1] << 16) |
			     (edp[canHdrOffset + CMP_CAN_HDR_ID_OFFSET + 2] << 8) |
			     (edp[canHdrOffset + CMP_CAN_HDR_ID_OFFSET + 3]));
		// const uint16_t errPos = ReadLE16(edp + canHdrOffset + CMP_CANFD_HDR_ERR_POS_OFFSET); // Reserved for future use
		// const unsigned char dlc = edp[canHdrOffset + CMP_CAN_HDR_DLC_OFFSET]; // Reserved for future use
		const unsigned char dataLen = edp[canHdrOffset + CMP_CAN_HDR_DATA_LEN_OFFSET];

		const size_t dataOffset = canHdrOffset + CMP_CAN_HDR_SIZE;
		if (edpLen < (dataOffset + (size_t)dataLen)) return false;

		// Extract CAN ID: lower 29 bits of big-endian ID word
		const uint32_t canId = (idWordBE & 0x1FFFFFFFu);
		// IDE bit is carried in the ID word; read explicitly (MSB)
		const bool ide = ((idWordBE & 0x80000000u) != 0);

		// Prepare output CAN FD VSB message
		memset(&outCan, 0, sizeof(outCan));
		outCan.Protocol = SPY_PROTOCOL_CANFD;
		outCan.NodeID = 0;
		outCan.NumberBytesHeader = 4; // ArbID in header
		outCan.MessagePieceID = 0;
		outCan.DescriptionID = 0;
		outCan.MiscData = 0;

		outCan.StatusBitField |= SPY_STATUS_NETWORK_MESSAGE_TYPE;
		outCan.StatusBitField |= SPY_STATUS_CANFD;
		if (ide) {
			outCan.StatusBitField |= SPY_STATUS_XTD_FRAME;
			outCan.StatusBitField3 |= SPY_STATUS3_CANFD_IDE;
		}
		// Direction: TX when dirOnIf set
		if ((commonFlags & CMP_COMMON_FLAG_DIR_ON_IF) != 0) {
			outCan.StatusBitField |= SPY_STATUS_TX_MSG;
		}

		// Map BRS, ESI, and set FDF in StatusBitField3
		if ((canfdFlags & 0x0010) != 0) outCan.StatusBitField3 |= SPY_STATUS3_CANFD_BRS;
		if ((canfdFlags & 0x0020) != 0) outCan.StatusBitField3 |= SPY_STATUS3_CANFD_ESI;
		outCan.StatusBitField3 |= SPY_STATUS3_CANFD_FDF;

		// Set network using generic mapping (fallback to sentinel)
		if (!SetCmpCanNetwork(outCan, cmpDeviceId, cmpInterfaceId)) {
			return false; // No valid network mapping, skip unwrapping
		}

		// Header holds ArbID
		outCan.ArbIDOrHeader = canId;

		// First up to 8 bytes in Data[8] (mirror first bytes)
		const unsigned int dataInHeader = (dataLen >= 8) ? 8u : (unsigned int)dataLen;
		for (unsigned int i = 0; i < dataInHeader; ++i)
			outCan.Data.data[i] = edp[dataOffset + i];
		// Per GOLDEN, NumberBytesData reflects total payload length
		outCan.NumberBytesData = dataLen;

		// Extra payload is the entire dataLen (GOLDEN shows ExtraData mirrors all 32 bytes)
		outCan.ExtraDataPtrEnabled = 1;
		outCan.ExtraDataPtr = dataLen;
		if (dataLen > 0) {
			extra.resize(dataLen);
			memcpy(extra.data(), edp + dataOffset, (size_t)dataLen);
		} else {
			extra.clear();
		}

		// Set timestamp based on timestampSource
		if (timestampSource == 0) {
			const uint64_t cmpTimestamp = ReadBE64(edp + dataHdrOffset + CMP_DATA_HDR_TIMESTAMP_OFFSET);
			CMessageTimeDecoderVSB::SetMessageTime(outCan, cmpTimestamp);
		} else {
			CopyTimestampFromOuter(inEth, outCan);
		}

		return true;
	};

	// Parse at standard CMP offset (after 14-byte Ethernet header)
	return parseAtOffset(CMP_HEADER_OFFSET);
}

// Ethernet segment reassembly context
struct EthernetSegmentContext {
	uint8_t streamId;
	uint16_t expectedSeqCounter;
	uint64_t timestamp;
	uint16_t deviceId;
	uint32_t interfaceId;
	std::vector<unsigned char> accumulatedData;
	bool active;

	EthernetSegmentContext() : streamId(0), expectedSeqCounter(0), timestamp(0), deviceId(0), interfaceId(0), active(false) {}
};

// Stream tracking for dropped frame detection
struct StreamTracker {
	uint16_t lastSeqCounter;
	bool initialized;

	StreamTracker() : lastSeqCounter(0), initialized(false) {}
};

// Composite key for stream tracking (DeviceId + StreamId)
struct StreamKey {
	uint16_t deviceId;
	uint8_t streamId;

	StreamKey(uint16_t dev, uint8_t stream) : deviceId(dev), streamId(stream) {}

	bool operator<(const StreamKey& other) const {
		if (deviceId != other.deviceId) return deviceId < other.deviceId;
		return streamId < other.streamId;
	}
};

// Dropped frame event record
struct DroppedFrameEvent {
	unsigned long long messageIndex;
	uint16_t deviceId;
	uint8_t streamId;
	uint16_t expectedSeq;
	uint16_t actualSeq;
	uint16_t droppedCount;
	double timestamp;
};

static bool TryUnwrapCmpEthernet(const icsSpyMessageVSB& srcMsg,
	const unsigned char* edp, const size_t edpLen,
	const int timestampSource,
	icsSpyMessageVSB& outMsg,
	std::vector<unsigned char>& outExtra,
	std::map<uint8_t, EthernetSegmentContext>& segmentContexts)
{
	const size_t cmpOffset = CMP_HEADER_OFFSET;
	if (edpLen < (cmpOffset + CMP_HEADER_SIZE + CMP_DATA_MESSAGE_HEADER_SIZE))
		return false;

	const size_t dataHdrOffset = cmpOffset + CMP_HEADER_SIZE;

	const uint16_t cmpDeviceId = ReadBE16(edp + cmpOffset + CMP_HDR_DEVICE_ID_OFFSET);
	const uint8_t streamId = edp[cmpOffset + CMP_HDR_STREAM_ID_OFFSET];
	const uint16_t streamSeqCounter = ReadBE16(edp + cmpOffset + CMP_HDR_SEQ_COUNTER_OFFSET);
	const uint32_t cmpInterfaceId = ReadLE32(edp + dataHdrOffset + CMP_DATA_HDR_INTERFACE_ID_OFFSET);
	const uint64_t cmpTimestamp = ReadBE64(edp + dataHdrOffset + CMP_DATA_HDR_TIMESTAMP_OFFSET);
	const uint8_t commonFlags = edp[dataHdrOffset + CMP_DATA_HDR_COMMON_FLAGS_OFFSET];
	const uint8_t payloadType = edp[dataHdrOffset + CMP_DATA_HDR_PAYLOAD_TYPE_OFFSET];

	if (payloadType != CMP_DATA_MESSAGE_PAYLOAD_TYPE_ETHERNET)
		return false;

	const uint16_t msgPayloadLength = ReadBE16(edp + dataHdrOffset + CMP_DATA_HDR_PAYLOAD_LEN_OFFSET);
	const size_t ethPayloadOffset = dataHdrOffset + CMP_DATA_MESSAGE_HEADER_SIZE;

	if (edpLen < (ethPayloadOffset + msgPayloadLength))
		return false;

	// Parse Ethernet payload structure: FLAGS(2) + RESERVED(2) + DATA_LENGTH(2) + DATA[N]
	if (msgPayloadLength < CMP_ETH_HDR_MIN_SIZE)
		return false;

	// const uint16_t ethFlags = ReadBE16(edp + ethPayloadOffset + CMP_ETH_HDR_FLAGS_OFFSET); // Reserved for future use
	const uint16_t ethDataLength = ReadBE16(edp + ethPayloadOffset + CMP_ETH_HDR_DATA_LEN_OFFSET);

	if (msgPayloadLength < (CMP_ETH_HDR_MIN_SIZE + ethDataLength))
		return false;

	const unsigned char* ethData = edp + ethPayloadOffset + CMP_ETH_HDR_DATA_OFFSET;

	// Extract SEG field (bits 6-7 of Common Flags)
	const uint8_t segType = commonFlags & CMP_SEG_MASK;

	if (segType == CMP_SEG_UNSEGMENTED)
	{
		// Complete unsegmented Ethernet frame
		memset(&outMsg, 0, sizeof(icsSpyMessageVSB));
		if (timestampSource == 0) {
			CMessageTimeDecoderVSB::SetMessageTime(outMsg, cmpTimestamp);
		}
		else {
			CopyTimestampFromOuter(srcMsg, outMsg);
		}

		outMsg.StatusBitField = SPY_STATUS_NETWORK_MESSAGE_TYPE;
		outMsg.Protocol = SPY_PROTOCOL_ETHERNET;
		outMsg.DescriptionID = 0;

		// Map network
		uint16_t netId16 = NETID_INVALID;
		if (!CmpMapLookup16(cmpDeviceId, cmpInterfaceId, netId16)) {
			return false; // No valid network mapping, skip unwrapping
		}
		SetNetworkID16(outMsg, netId16);

		// For Ethernet: Data[8] is EMPTY, all data goes to ExtraData
		memset(outMsg.Data.data, 0, 8);
		outMsg.NumberBytesData = (uint8_t)(ethDataLength & 0xFF);
		outMsg.ExtraDataPtrEnabled = 1;
		outMsg.ExtraDataPtr = ethDataLength;

		// Copy Ethernet frame to extra buffer
		outExtra.resize(ethDataLength);
		memcpy(&outExtra[0], ethData, ethDataLength);

		return true;
	}
	else if (segType == CMP_SEG_START)
	{
		// Start of new segmented message
		EthernetSegmentContext& ctx = segmentContexts[streamId];
		ctx.active = true;
		ctx.streamId = streamId;
		ctx.expectedSeqCounter = streamSeqCounter + 1;
		ctx.timestamp = cmpTimestamp;
		ctx.deviceId = cmpDeviceId;
		ctx.interfaceId = cmpInterfaceId;
		ctx.accumulatedData.clear();
		ctx.accumulatedData.insert(ctx.accumulatedData.end(), ethData, ethData + ethDataLength);
		return false; // Not complete yet
	}
	else if (segType == CMP_SEG_CONTINUATION)
	{
		// Middle segment
		auto it = segmentContexts.find(streamId);
		if (it == segmentContexts.end() || !it->second.active)
			return false; // No active context

		EthernetSegmentContext& ctx = it->second;
		// Validate sequence
		if (streamSeqCounter != ctx.expectedSeqCounter)
		{
			ctx.active = false; // Sequence error, abandon
			return false;
		}

		ctx.accumulatedData.insert(ctx.accumulatedData.end(), ethData, ethData + ethDataLength);
		ctx.expectedSeqCounter++;
		return false; // Not complete yet
	}
	else if (segType == CMP_SEG_END)
	{
		// Last segment - complete the frame
		auto it = segmentContexts.find(streamId);
		if (it == segmentContexts.end() || !it->second.active)
			return false; // No active context

		EthernetSegmentContext& ctx = it->second;
		// Validate sequence
		if (streamSeqCounter != ctx.expectedSeqCounter)
		{
			ctx.active = false; // Sequence error, abandon
			return false;
		}

		// Append final segment
		ctx.accumulatedData.insert(ctx.accumulatedData.end(), ethData, ethData + ethDataLength);

		// Build complete Ethernet message
		memset(&outMsg, 0, sizeof(icsSpyMessageVSB));
		if (timestampSource == 0) {
			CMessageTimeDecoderVSB::SetMessageTime(outMsg, ctx.timestamp);
		}
		else {
			CopyTimestampFromOuter(srcMsg, outMsg);
		}

		outMsg.StatusBitField = SPY_STATUS_NETWORK_MESSAGE_TYPE;
		outMsg.Protocol = SPY_PROTOCOL_ETHERNET;
		outMsg.DescriptionID = 0;

		// Map network
		uint16_t netId16 = NETID_INVALID;
		if (!CmpMapLookup16(ctx.deviceId, ctx.interfaceId, netId16)) {
			ctx.active = false;
			ctx.accumulatedData.clear();
			return false; // No valid network mapping, skip unwrapping
		}
		SetNetworkID16(outMsg, netId16);

		// For Ethernet: Data[8] is EMPTY, all data goes to ExtraData
		const uint32_t totalLen = (uint32_t)ctx.accumulatedData.size();
		memset(outMsg.Data.data, 0, 8);
		outMsg.NumberBytesData = (uint8_t)(totalLen & 0xFF);
		outMsg.ExtraDataPtrEnabled = 1;
		outMsg.ExtraDataPtr = totalLen;

		// Copy complete Ethernet frame to extra buffer
		outExtra = ctx.accumulatedData;

		// Clear context
		ctx.active = false;
		ctx.accumulatedData.clear();

		return true;
	}

	return false;
}

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
bool UnwrapCMPToNative(const char* inputFilePath, const char* outputFilePath, const int sortOutput, const int timestampSource, ProgressFunc prog, const int outputInfoToTxt)
{
	VSBIORead reader(inputFilePath);
	VSBIOWrite writer;
	if (!writer.Init(outputFilePath))
		return false;
 
	unsigned long long counter = 0ULL;
	std::vector<unsigned char> msg;
	std::map<uint8_t, EthernetSegmentContext> segmentContexts;
	std::map<StreamKey, StreamTracker> streamTrackers;
	std::vector<DroppedFrameEvent> droppedFrames;

	// Timestamp-sorted output buffer (includes CMP and non-CMP messages)
	struct PendingMessage
	{
		uint64_t key; // Combined timestamp key: TimeHardware2:TimeHardware
		std::vector<unsigned char> bytes;
	};

	constexpr size_t kBufferCapacity = 128 * 1024; // messages
	constexpr size_t kFlushSize = kBufferCapacity / 2; // flush half when full
	std::vector<PendingMessage> outBuffer;
	outBuffer.reserve(kBufferCapacity);

	auto makeKey = [](const icsSpyMessageVSB& m) -> uint64_t {
		return (static_cast<uint64_t>(m.TimeHardware2) << 32) | static_cast<uint64_t>(m.TimeHardware);
	};

	auto flushBuffer = [&](const bool flushAll) {
		if (outBuffer.empty()) return;
		if (sortOutput == 0) { // Sort frames by timestamp
			std::stable_sort(outBuffer.begin(), outBuffer.end(), [](const PendingMessage& a, const PendingMessage& b) {
				return a.key < b.key;
			});
		}
		const size_t toWrite = flushAll ? outBuffer.size() : kFlushSize;
		for (size_t i = 0; i < toWrite; ++i)
			writer.WriteMessage(outBuffer[i].bytes);
		outBuffer.erase(outBuffer.begin(), outBuffer.begin() + (ptrdiff_t)toWrite);
	};

	auto pushMessage = [&](std::vector<unsigned char>&& bytes) {
		uint64_t key = 0;
		if (bytes.size() >= sizeof(icsSpyMessageVSB))
		{
			const icsSpyMessageVSB* m = reinterpret_cast<const icsSpyMessageVSB*>(&bytes[0]);
			key = makeKey(*m);
		}
		PendingMessage pm{ key, std::move(bytes) };
		outBuffer.emplace_back(std::move(pm));
		if (outBuffer.size() >= kBufferCapacity)
			flushBuffer(false); // flush half when full to keep memory bounded
	};

	while (reader.ReadNextMessage(msg) == VSBIORead::eSuccess)
	{
		if (prog && !((++counter) % 100000))
		{
			if (!prog(reader.GetProgress()))
				break;
		}

		if (msg.size() < sizeof(icsSpyMessageVSB))
		{
			// Corrupt, write as-is
			pushMessage(std::vector<unsigned char>(msg));
			continue;
		}

		icsSpyMessageVSB* pMsg = reinterpret_cast<icsSpyMessageVSB*>(&msg[0]);

		bool wroteCMP = false;
		if (pMsg->Protocol == SPY_PROTOCOL_ETHERNET && pMsg->ExtraDataPtrEnabled && pMsg->ExtraDataPtr)
		{
			const unsigned char* edp = &msg[sizeof(icsSpyMessageVSB)];
			const size_t edpLen = (size_t)pMsg->ExtraDataPtr; //ExtraDataPtr is a size in this context, not a pointer

			// Detect CMP ethertype
			if (edpLen >= CMP_HEADER_OFFSET)
			{
				const unsigned short ethertype = ReadBE16(edp + ETH_ETHERTYPE_OFFSET);
				if (ethertype == CMP_ETHERTYPE)
				{
					// Parse CMP header at standard offset (after 14-byte Ethernet header)
					const size_t cmpOffset = CMP_HEADER_OFFSET;
					if (edpLen >= (cmpOffset + CMP_HEADER_SIZE + CMP_DATA_MESSAGE_HEADER_SIZE))
					{
						const unsigned char cmpVersion = edp[cmpOffset + CMP_HDR_VERSION_OFFSET];
						if (cmpVersion == CMP_VERSION)
						{
							// Extract stream information for dropped frame detection
							// NOTE: Sequence counter increments for ALL CMP message types, not just CAP_DATA_MSG
							const uint16_t cmpDeviceId = ReadBE16(edp + cmpOffset + CMP_HDR_DEVICE_ID_OFFSET);
							const unsigned char cmpMessageType = edp[cmpOffset + CMP_HDR_MSG_TYPE_OFFSET];
							const uint8_t streamId = edp[cmpOffset + CMP_HDR_STREAM_ID_OFFSET];
							const uint16_t streamSeqCounter = ReadBE16(edp + cmpOffset + CMP_HDR_SEQ_COUNTER_OFFSET);

							// Check for dropped frames (sequence counter is per DeviceId+StreamId)
							// Track ALL CMP message types to detect drops correctly
							StreamKey streamKey(cmpDeviceId, streamId);
							StreamTracker& tracker = streamTrackers[streamKey];
							if (tracker.initialized)
							{
								// Expected next sequence counter (wraps at 65536)
const uint16_t expectedSeq = (uint16_t)(tracker.lastSeqCounter + 1);
							if (streamSeqCounter != expectedSeq)
							{
								// Detected a gap - calculate number of dropped frames
								const uint16_t droppedCount = (streamSeqCounter > expectedSeq)
									? (uint16_t)(streamSeqCounter - expectedSeq)
									: (uint16_t)(0xFFFF - expectedSeq + streamSeqCounter + 1);

									// Record dropped frame event
									DroppedFrameEvent evt;
									evt.messageIndex = counter;
									evt.deviceId = cmpDeviceId;
									evt.streamId = streamId;
									evt.expectedSeq = expectedSeq;
									evt.actualSeq = streamSeqCounter;
									evt.droppedCount = droppedCount;
									evt.timestamp = CMessageTimeDecoderVSB::CalcTimeStamp(*pMsg);
									droppedFrames.push_back(evt);
								}
							}
							else
							{
								tracker.initialized = true;
							}
							tracker.lastSeqCounter = streamSeqCounter;

							// Only unwrap and export CAP_DATA_MSG, but pass through all as Ethernet
							if (cmpMessageType != CMP_MESSAGE_TYPE_CAP_DATA)
							{
								// Non-data message (e.g., StatusMessage 0x03) - pass through as Ethernet
								pushMessage(std::vector<unsigned char>(msg));
								continue;
							}

							const size_t dataHdrOffset = cmpOffset + CMP_HEADER_SIZE;
							const unsigned char payloadType = edp[dataHdrOffset + CMP_DATA_HDR_PAYLOAD_TYPE_OFFSET];

							// Dispatch based on payload type
							switch (payloadType)
							{
							case CMP_DATA_MESSAGE_PAYLOAD_TYPE_CAN:
							{
								icsSpyMessageVSB can{};
								std::vector<unsigned char> extra;
								if (TryUnwrapCmpCan(*pMsg, edp, edpLen, timestampSource, can, extra))
								{
									std::vector<unsigned char> out(sizeof(icsSpyMessageVSB));
									memcpy(&out[0], &can, sizeof(icsSpyMessageVSB));
									pushMessage(std::move(out));
									wroteCMP = true;
								}
								break;
							}
							case CMP_DATA_MESSAGE_PAYLOAD_TYPE_LIN:
							{
								icsSpyMessageVSB lin{};
								if (TryUnwrapCmpLin(*pMsg, edp, edpLen, timestampSource, lin))
								{
									std::vector<unsigned char> out(sizeof(icsSpyMessageVSB));
									memcpy(&out[0], &lin, sizeof(icsSpyMessageVSB));
									pushMessage(std::move(out));
									wroteCMP = true;
								}
								break;
							}
							case CMP_DATA_MESSAGE_PAYLOAD_TYPE_ETHERNET:
							{
								icsSpyMessageVSB ethernet{};
								std::vector<unsigned char> extra;
								if (TryUnwrapCmpEthernet(*pMsg, edp, edpLen, timestampSource, ethernet, extra, segmentContexts))
								{
									std::vector<unsigned char> out(sizeof(icsSpyMessageVSB) + extra.size());
									memcpy(&out[0], &ethernet, sizeof(icsSpyMessageVSB));
									if (!extra.empty()) memcpy(&out[sizeof(icsSpyMessageVSB)], &extra[0], extra.size());
									pushMessage(std::move(out));
									wroteCMP = true;
								}
								break;
							}
							case CMP_DATA_MESSAGE_PAYLOAD_TYPE_CANFD:
							{
								icsSpyMessageVSB canfd{};
								std::vector<unsigned char> extra;
								if (TryUnwrapCmpCanFd(*pMsg, edp, edpLen, timestampSource, canfd, extra))
								{
									std::vector<unsigned char> out(sizeof(icsSpyMessageVSB) + extra.size());
									memcpy(&out[0], &canfd, sizeof(icsSpyMessageVSB));
									if (!extra.empty()) memcpy(&out[sizeof(icsSpyMessageVSB)], &extra[0], extra.size());
									pushMessage(std::move(out));
									wroteCMP = true;
								}
								break;
							}
							default:
								// Unknown or unsupported payload type - pass through
								break;
							}
						}
					}
				}
			}
		}

		// Pass-through non-LIN/non-CMP messages to keep original Ethernet and others
		if (!wroteCMP)
		{
			pushMessage(std::vector<unsigned char>(msg));
		}
	}

	// Flush any remaining buffered messages in timestamp order
	flushBuffer(true);

	// Write dropped frame report if any were detected (when enabled)
	if (outputInfoToTxt && !droppedFrames.empty())
	{
		// Create report filename based on output filename: <outputfilename>.txt
		std::string infoFilePath = outputFilePath;
		const size_t lastDot = infoFilePath.find_last_of(".");
		if (lastDot != std::string::npos)
			infoFilePath = infoFilePath.substr(0, lastDot) + ".txt";
		else
			infoFilePath = infoFilePath + ".txt";

		std::ofstream infoFile(infoFilePath, std::ios::out | std::ios::trunc);
		if (infoFile)
		{
			infoFile << "CMP Dropped Message Detection Report\n";
			infoFile << "====================================\n";
			infoFile << "Input File: " << inputFilePath << "\n";
			infoFile << "Output File: " << outputFilePath << "\n";
			infoFile << "Total Messages Processed: " << counter << "\n";
			infoFile << "Total Dropped Message Events: " << droppedFrames.size() << "\n\n";

			// Calculate total dropped messages across all events
			unsigned long long totalDropped = 0;
			for (const auto& evt : droppedFrames)
				totalDropped += evt.droppedCount;
			infoFile << "Total Messages Dropped: " << totalDropped << "\n";
			
			// Calculate percentage of messages dropped (out of total expected messages)
			if (counter > 0)
			{
				const unsigned long long totalExpectedFrames = counter + totalDropped;
				const double dropPercentage = (static_cast<double>(totalDropped) / static_cast<double>(totalExpectedFrames)) * 100.0;
				infoFile << "Percentage of Messages Dropped: " << std::fixed << std::setprecision(4) << dropPercentage << "%\n";
			}
			infoFile << "\n";

			infoFile << "Detailed Event Log:\n";
			infoFile << "-------------------\n";
			infoFile << std::fixed << std::setprecision(6);

			for (size_t i = 0; i < droppedFrames.size(); ++i)
			{
				const DroppedFrameEvent& evt = droppedFrames[i];
				infoFile << "Event " << (i + 1) << ":\n";
				infoFile << "  Message Index: " << evt.messageIndex << "\n";
				infoFile << "  Timestamp: " << evt.timestamp << " seconds\n";
				infoFile << "  Device ID: " << evt.deviceId << " (0x" << std::hex << std::uppercase << evt.deviceId << std::dec << ")\n";
				infoFile << "  Stream ID: " << (unsigned int)evt.streamId << "\n";
				infoFile << "  Expected Sequence: " << evt.expectedSeq << " (0x" << std::hex << std::uppercase << evt.expectedSeq << std::dec << ")\n";
				infoFile << "  Actual Sequence: " << evt.actualSeq << " (0x" << std::hex << std::uppercase << evt.actualSeq << std::dec << ")\n";
				infoFile << "  Messages Dropped: " << evt.droppedCount << " (0x" << std::hex << std::uppercase << evt.droppedCount << std::dec << ")\n\n";
			}

			infoFile.flush();
		}
	}

	return true;
}

