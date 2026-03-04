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

static inline void SetNetworkID16(icsSpyMessageVSB& msg, unsigned short netid16)
{
	msg.NetworkID = (unsigned char)(netid16 & 0xFF);
	msg.NetworkID2 = (unsigned char)((netid16 >> 8) & 0xFF);
}

// Generic CMP→VSB network mapping: try DeviceId+InterfaceId mapping table,
// then fall back to a provided 16-bit NetworkID when available.
// Returns true if a valid network mapping was found or fallback was applied; false otherwise.
static inline bool SetCmpNetworkFromMap(icsSpyMessageVSB& msg, uint16_t cmpDeviceId, uint32_t cmpInterfaceId, unsigned short fallbackNetId)
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
#define CMP_HEADER_OFFSET 14
#define CMP_HEADER_SIZE 8
#define CMP_DATA_MESSAGE_HEADER_SIZE 16
#define CMP_DATA_MESSAGE_PAYLOAD_TYPE_CAN 0x01
#define CMP_DATA_MESSAGE_PAYLOAD_TYPE_CANFD 0x02
#define CMP_DATA_MESSAGE_PAYLOAD_TYPE_LIN 0x03
#define CMP_DATA_MESSAGE_PAYLOAD_TYPE_ETHERNET 0x08

// SEG field values (bits 6-7 of Common Flags)
#define CMP_SEG_UNSEGMENTED 0x00
#define CMP_SEG_START 0x40       // 0b01000000
#define CMP_SEG_CONTINUATION 0x80 // 0b10000000
#define CMP_SEG_END 0xC0          // 0b11000000
#define CMP_SEG_MASK 0xC0

// Heuristic stub: we currently only detect CMP ethertype and version; detailed LIN payload parsing will follow.
static bool TryUnwrapCmpLin(const icsSpyMessageVSB& inEth, const unsigned char* edp, size_t edpLen,
	int timestampSource, icsSpyMessageVSB& outLin)
{
	const unsigned short ethertype = ReadBE16(edp + 12);
	if (ethertype != 0x99FE) return false;

	auto parseAtOffset = [&](size_t cmpOffset) -> bool {
		if (edpLen < (cmpOffset + CMP_HEADER_SIZE + CMP_DATA_MESSAGE_HEADER_SIZE)) return false;

		const unsigned char cmpVersion = edp[cmpOffset + 0];
		if (cmpVersion != 0x01) return false;

		const size_t dataHdrOffset = cmpOffset + CMP_HEADER_SIZE;
		const uint16_t cmpDeviceId = ReadBE16(edp + cmpOffset + 2); // BE16 DeviceId
		const uint32_t cmpInterfaceId = ReadLE32(edp + dataHdrOffset + 8); // LE32 InterfaceId
		const unsigned char commonFlags = edp[dataHdrOffset + 12];
		const unsigned char payloadType = edp[dataHdrOffset + 13];
		if (payloadType != CMP_DATA_MESSAGE_PAYLOAD_TYPE_LIN) return false;

		const size_t linHdrOffset = dataHdrOffset + CMP_DATA_MESSAGE_HEADER_SIZE;
		if (edpLen < (linHdrOffset + 8)) return false;

		const uint16_t linFlags = ReadLE16(edp + linHdrOffset + 0);
		const unsigned char pid = edp[linHdrOffset + 4];
		const unsigned char crc = edp[linHdrOffset + 6];
		const unsigned char dataLen = edp[linHdrOffset + 7];

		const size_t linDataOffset = linHdrOffset + 8;
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
		unsigned char linId = (unsigned char)(pid & 0x3F);
		unsigned char* hdrBytes = reinterpret_cast<unsigned char*>(&outLin.ArbIDOrHeader);
		hdrBytes[0] = linId;

		// Historical VSPY behavior: store up to first 2 data bytes in header after ID
		unsigned char headerDataBytes = (dataLen < 2) ? dataLen : 2;
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
			unsigned int remaining = (unsigned int)dataLen - headerDataBytes;
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
		if ((commonFlags & 0x10) != 0)
			outLin.StatusBitField |= SPY_STATUS_TX_MSG;

		// Place checksum in AckBytes[0] per BLF import convention
		outLin.AckBytes.data[0] = crc;

		// Set timestamp based on timestampSource
		if (timestampSource == 0) {
			// Use CMP timestamp (LIN does not have its own, so use the one from the CMP header)
			const uint64_t cmpTimestamp = ReadBE64(edp + dataHdrOffset + 0);
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
static inline bool SetCmpCanNetwork(icsSpyMessageVSB& msg, uint16_t cmpDeviceId, uint32_t cmpIf)
{
	return SetCmpNetworkFromMap(msg, cmpDeviceId, cmpIf, NETID_INVALID);
}

static bool TryUnwrapCmpCan(const icsSpyMessageVSB& inEth, const unsigned char* edp, size_t edpLen,
	int timestampSource, icsSpyMessageVSB& outCan, std::vector<unsigned char>& extra)
{
	const unsigned short ethertype = ReadBE16(edp + 12);
	if (ethertype != 0x99FE) return false;

	auto parseAtOffset = [&](size_t cmpOffset) -> bool {
		if (edpLen < (cmpOffset + CMP_HEADER_SIZE + CMP_DATA_MESSAGE_HEADER_SIZE)) return false;

		const unsigned char cmpVersion = edp[cmpOffset + 0];
		if (cmpVersion != 0x01) return false;

		const size_t dataHdrOffset = cmpOffset + CMP_HEADER_SIZE;
		const uint16_t cmpDeviceId = ReadBE16(edp + cmpOffset + 2); // BE16 DeviceId
		const uint32_t cmpInterfaceId = ReadLE32(edp + dataHdrOffset + 8); // LE32 InterfaceId
		const unsigned char commonFlags = edp[dataHdrOffset + 12];
		const unsigned char payloadType = edp[dataHdrOffset + 13];
		if (payloadType != CMP_DATA_MESSAGE_PAYLOAD_TYPE_CAN) return false;

		const size_t canHdrOffset = dataHdrOffset + CMP_DATA_MESSAGE_HEADER_SIZE;
		if (edpLen < (canHdrOffset + 16)) return false;

		// const uint16_t canFlags = ReadLE16(edp + canHdrOffset + 0); // Reserved for future use
		// CAN ID word: bytes appear reversed on transmit; interpret as big-endian here
		const uint32_t idWordBE = (uint32_t)((edp[canHdrOffset + 4] << 24) |
			     (edp[canHdrOffset + 5] << 16) |
			     (edp[canHdrOffset + 6] << 8) |
			     (edp[canHdrOffset + 7]));
		// const unsigned char dlc = edp[canHdrOffset + 14]; // Reserved for future use
		const unsigned char dataLen = edp[canHdrOffset + 15];

		// CAN max 8 bytes
		if (dataLen > 8) return false;

		const size_t dataOffset = canHdrOffset + 16;
		if (edpLen < (dataOffset + (size_t)dataLen)) return false;

		// Extract CAN ID: lower 29 bits of big-endian ID word
		uint32_t canId = (idWordBE & 0x1FFFFFFFu);
		// IDE bit is carried in the ID word; read explicitly (MSB)
		bool ide = ((idWordBE & 0x80000000u) != 0);
		// RTR bit for remote frames
		bool rtr = ((idWordBE & 0x40000000u) != 0);

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
		if ((commonFlags & 0x10) != 0) {
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
			const uint64_t cmpTimestamp = ReadBE64(edp + dataHdrOffset + 0);
			CMessageTimeDecoderVSB::SetMessageTime(outCan, cmpTimestamp);
		} else {
			CopyTimestampFromOuter(inEth, outCan);
		}

		return true;
	};

	// Parse at standard CMP offset (after 14-byte Ethernet header)
	return parseAtOffset(CMP_HEADER_OFFSET);
}

static bool TryUnwrapCmpCanFd(const icsSpyMessageVSB& inEth, const unsigned char* edp, size_t edpLen,
	int timestampSource, icsSpyMessageVSB& outCan, std::vector<unsigned char>& extra)
{
	// timestampSource is now used below
	const unsigned short ethertype = ReadBE16(edp + 12);
	if (ethertype != 0x99FE) return false;

	auto parseAtOffset = [&](size_t cmpOffset) -> bool {
		if (edpLen < (cmpOffset + CMP_HEADER_SIZE + CMP_DATA_MESSAGE_HEADER_SIZE)) return false;

		const unsigned char cmpVersion = edp[cmpOffset + 0];
		if (cmpVersion != 0x01) return false;

		const size_t dataHdrOffset = cmpOffset + CMP_HEADER_SIZE;
		const uint16_t cmpDeviceId = ReadBE16(edp + cmpOffset + 2); // BE16 DeviceId
		const uint32_t cmpInterfaceId = ReadLE32(edp + dataHdrOffset + 8); // LE32 InterfaceId
		const unsigned char commonFlags = edp[dataHdrOffset + 12];
		const unsigned char payloadType = edp[dataHdrOffset + 13];
		if (payloadType != CMP_DATA_MESSAGE_PAYLOAD_TYPE_CANFD) return false;

		const size_t canHdrOffset = dataHdrOffset + CMP_DATA_MESSAGE_HEADER_SIZE;
		if (edpLen < (canHdrOffset + 16)) return false;

		const uint16_t canfdFlags = ReadLE16(edp + canHdrOffset + 0);
		// const uint16_t reserved = ReadLE16(edp + canHdrOffset + 2);
		// CAN FD ID word: bytes appear reversed on transmit; interpret as big-endian here
		const uint32_t idWordBE = (uint32_t)((edp[canHdrOffset + 4] << 24) |
			     (edp[canHdrOffset + 5] << 16) |
			     (edp[canHdrOffset + 6] << 8) |
			     (edp[canHdrOffset + 7]));
		// const uint16_t errPos = ReadLE16(edp + canHdrOffset + 12); // Reserved for future use
		// const unsigned char dlc = edp[canHdrOffset + 14]; // Reserved for future use
		const unsigned char dataLen = edp[canHdrOffset + 15];

		const size_t dataOffset = canHdrOffset + 16;
		if (edpLen < (dataOffset + (size_t)dataLen)) return false;

		// Extract CAN ID: lower 29 bits of big-endian ID word
		uint32_t canId = (idWordBE & 0x1FFFFFFFu);
		// IDE bit is carried in the ID word; read explicitly (MSB)
		bool ide = ((idWordBE & 0x80000000u) != 0);

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
		if ((commonFlags & 0x10) != 0) {
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
		unsigned int dataInHeader = (dataLen >= 8) ? 8u : (unsigned int)dataLen;
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
			const uint64_t cmpTimestamp = ReadBE64(edp + dataHdrOffset + 0);
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
	const unsigned char* edp, size_t edpLen,
	int timestampSource,
	icsSpyMessageVSB& outMsg,
	std::vector<unsigned char>& outExtra,
	std::map<uint8_t, EthernetSegmentContext>& segmentContexts)
{
	const size_t cmpOffset = CMP_HEADER_OFFSET;
	if (edpLen < (cmpOffset + CMP_HEADER_SIZE + CMP_DATA_MESSAGE_HEADER_SIZE))
		return false;

	const size_t dataHdrOffset = cmpOffset + CMP_HEADER_SIZE;

	const uint16_t cmpDeviceId = ReadBE16(edp + cmpOffset + 2);
	const uint8_t streamId = edp[cmpOffset + 5];
	const uint16_t streamSeqCounter = ReadBE16(edp + cmpOffset + 6);
	const uint32_t cmpInterfaceId = ReadLE32(edp + dataHdrOffset + 8); // LE32 InterfaceId
	const uint64_t cmpTimestamp = ReadBE64(edp + dataHdrOffset + 0);
	const uint8_t commonFlags = edp[dataHdrOffset + 12];
	const uint8_t payloadType = edp[dataHdrOffset + 13];

	if (payloadType != CMP_DATA_MESSAGE_PAYLOAD_TYPE_ETHERNET)
		return false;

	const uint16_t msgPayloadLength = ReadBE16(edp + dataHdrOffset + 14);
	const size_t ethPayloadOffset = dataHdrOffset + CMP_DATA_MESSAGE_HEADER_SIZE;

	if (edpLen < (ethPayloadOffset + msgPayloadLength))
		return false;

	// Parse Ethernet payload structure: FLAGS(2) + RESERVED(2) + DATA_LENGTH(2) + DATA[N]
	if (msgPayloadLength < 6)
		return false;

	// const uint16_t ethFlags = ReadBE16(edp + ethPayloadOffset + 0); // Reserved for future use
	const uint16_t ethDataLength = ReadBE16(edp + ethPayloadOffset + 4);

	if (msgPayloadLength < (6 + ethDataLength))
		return false;

	const unsigned char* ethData = edp + ethPayloadOffset + 6;

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
		uint32_t totalLen = (uint32_t)ctx.accumulatedData.size();
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
bool UnwrapCMPToNative(const char* inputFilePath, const char* outputFilePath, int sortOutput, int timestampSource, ProgressFunc prog, int outputInfoToTxt)
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

	auto flushBuffer = [&](bool flushAll) {
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
			icsSpyMessageVSB* m = reinterpret_cast<icsSpyMessageVSB*>(&bytes[0]);
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
			if (edpLen >= 14)
			{
				const unsigned short ethertype = ReadBE16(edp + 12);
				if (ethertype == 0x99FE)
				{
					// Parse CMP header at standard offset (after 14-byte Ethernet header)
					const size_t cmpOffset = CMP_HEADER_OFFSET;
					if (edpLen >= (cmpOffset + CMP_HEADER_SIZE + CMP_DATA_MESSAGE_HEADER_SIZE))
					{
						const unsigned char cmpVersion = edp[cmpOffset + 0];
						if (cmpVersion == 0x01)
						{
							// Extract stream information for dropped frame detection
							// NOTE: Sequence counter increments for ALL CMP message types, not just CAP_DATA_MSG
							const uint16_t cmpDeviceId = ReadBE16(edp + cmpOffset + 2);
							const unsigned char cmpMessageType = edp[cmpOffset + 4];
							const uint8_t streamId = edp[cmpOffset + 5];
							const uint16_t streamSeqCounter = ReadBE16(edp + cmpOffset + 6);

							// Check for dropped frames (sequence counter is per DeviceId+StreamId)
							// Track ALL CMP message types to detect drops correctly
							StreamKey streamKey(cmpDeviceId, streamId);
							StreamTracker& tracker = streamTrackers[streamKey];
							if (tracker.initialized)
							{
								// Expected next sequence counter (wraps at 65536)
								uint16_t expectedSeq = (uint16_t)(tracker.lastSeqCounter + 1);
								if (streamSeqCounter != expectedSeq)
								{
									// Detected a gap - calculate number of dropped frames
									uint16_t droppedCount;
									if (streamSeqCounter > expectedSeq)
										droppedCount = streamSeqCounter - expectedSeq;
									else // Wrapped around
										droppedCount = (uint16_t)(0xFFFF - expectedSeq + streamSeqCounter + 1);

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

							// Only unwrap and export CAP_DATA_MSG (0x01), but pass through all as Ethernet
							if (cmpMessageType != 0x01)
							{
								// Non-data message (e.g., StatusMessage 0x03) - pass through as Ethernet
								pushMessage(std::vector<unsigned char>(msg));
								continue;
							}

							const size_t dataHdrOffset = cmpOffset + CMP_HEADER_SIZE;
							const unsigned char payloadType = edp[dataHdrOffset + 13];

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
		size_t lastDot = infoFilePath.find_last_of(".");
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
				unsigned long long totalExpectedFrames = counter + totalDropped;
				double dropPercentage = (static_cast<double>(totalDropped) / static_cast<double>(totalExpectedFrames)) * 100.0;
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


extern "C" VSBIODLL_API bool DumpVSBMessageFields(const char* inputFilePath, const char* outputTextFile, unsigned int maxMessages)
{
	VSBIORead reader(inputFilePath);
	std::ofstream out(outputTextFile, std::ios::out | std::ios::trunc);
	if (!out)
		return false;

	auto fmtHex = [](std::ostream& o, uint64_t v, int width) {
		std::ios_base::fmtflags f = o.flags();
		o << "0x" << std::uppercase << std::hex << std::setw(width) << std::setfill('0') << v;
		o.flags(f);
		o << std::setfill(' ');
	};

	auto bytesHex = [&](const uint8_t* p, size_t n) {
		std::ios_base::fmtflags f = out.flags();
		out << std::uppercase << std::hex;
		for (size_t i = 0; i < n; ++i) {
			out << std::setw(2) << std::setfill('0') << (unsigned int)p[i];
			if (i + 1 < n) out << ' ';
		}
		out.flags(f);
		out << std::setfill(' ');
	};

	out << "Dump of icsSpyMessageVSB fields\n";
	out << "Input: " << inputFilePath << "\n";
	out << "Note: arrays shown in hex, integers shown as dec (hex)\n\n";

	std::vector<unsigned char> msg;
	unsigned long long index = 0ULL;
	while (reader.ReadNextMessage(msg) == VSBIORead::eSuccess)
	{
		if (maxMessages && index >= maxMessages) break;
		++index;

		out << "----- Message " << index << " size=" << msg.size() << " -----\n";
		if (msg.size() < sizeof(icsSpyMessageVSB))
		{
			out << "Corrupt: size < icsSpyMessageVSB\n\n";
			continue;
		}

		icsSpyMessageVSB* m = reinterpret_cast<icsSpyMessageVSB*>(&msg[0]);

		// Top-level fields
		out << "StatusBitField: " << (uint32_t)m->StatusBitField << " ("; fmtHex(out, m->StatusBitField, 8); out << ")\n";
		out << "StatusBitField2: " << (uint32_t)m->StatusBitField2 << " ("; fmtHex(out, m->StatusBitField2, 8); out << ")\n";
		out << "TimeHardware: " << (uint32_t)m->TimeHardware << " ("; fmtHex(out, m->TimeHardware, 8); out << ")\n";
		out << "TimeHardware2: " << (uint32_t)m->TimeHardware2 << " ("; fmtHex(out, m->TimeHardware2, 8); out << ")\n";
		out << "TimeSystem: " << (uint32_t)m->TimeSystem << " ("; fmtHex(out, m->TimeSystem, 8); out << ")\n";
		out << "TimeSystem2: " << (uint32_t)m->TimeSystem2 << " ("; fmtHex(out, m->TimeSystem2, 8); out << ")\n";
		out << "TimeStampHardwareID: " << (unsigned int)m->TimeStampHardwareID << " ("; fmtHex(out, m->TimeStampHardwareID, 2); out << ")\n";
		out << "TimeStampSystemID: " << (unsigned int)m->TimeStampSystemID << " ("; fmtHex(out, m->TimeStampSystemID, 2); out << ")\n";
		out << "NetworkID: " << (unsigned int)m->NetworkID << " ("; fmtHex(out, m->NetworkID, 2); out << ")\n";
		out << "NodeID: " << (unsigned int)m->NodeID << " ("; fmtHex(out, m->NodeID, 2); out << ")\n";
		out << "Protocol: " << (unsigned int)m->Protocol << " ("; fmtHex(out, m->Protocol, 2); out << ")\n";
		out << "MessagePieceID: " << (unsigned int)m->MessagePieceID << " ("; fmtHex(out, m->MessagePieceID, 2); out << ")\n";
		out << "ExtraDataPtrEnabled: " << (unsigned int)m->ExtraDataPtrEnabled << " ("; fmtHex(out, m->ExtraDataPtrEnabled, 2); out << ")\n";
		out << "NumberBytesHeader: " << (unsigned int)m->NumberBytesHeader << " ("; fmtHex(out, m->NumberBytesHeader, 2); out << ")\n";
		out << "NumberBytesData: " << (unsigned int)m->NumberBytesData << " ("; fmtHex(out, m->NumberBytesData, 2); out << ")\n";
		out << "NetworkID2: " << (unsigned int)m->NetworkID2 << " ("; fmtHex(out, m->NetworkID2, 2); out << ")\n";
		out << "DescriptionID: " << (int)m->DescriptionID << " ("; fmtHex(out, (uint16_t)m->DescriptionID, 4); out << ")\n";
		out << "ArbIDOrHeader: " << (uint32_t)m->ArbIDOrHeader << " ("; fmtHex(out, m->ArbIDOrHeader, 8); out << ")\n";

		out << "Data[8]: "; bytesHex(m->Data.data, 8); out << "\n";

		// Union view
		out << "StatusBitField3: " << (uint32_t)m->StatusBitField3 << " ("; fmtHex(out, m->StatusBitField3, 8); out << ")\n";
		out << "StatusBitField4: " << (uint32_t)m->StatusBitField4 << " ("; fmtHex(out, m->StatusBitField4, 8); out << ")\n";
		out << "AckBytes[8]: "; bytesHex(m->AckBytes.data, 8); out << "\n";

		out << "ExtraDataPtr (payload length): " << (uint32_t)m->ExtraDataPtr << " ("; fmtHex(out, m->ExtraDataPtr, 8); out << ")\n";
		out << "MiscData: " << (unsigned int)m->MiscData << " ("; fmtHex(out, m->MiscData, 2); out << ")\n";
		out << "Reserved[3]: "; bytesHex(m->Reserved.data, 3); out << "\n";

		// Derived timestamp (seconds since epoch base used by VSB)
		double ts = CMessageTimeDecoderVSB::CalcTimeStamp(*m);
		out << "DerivedTimestamp: " << std::fixed << std::setprecision(6) << ts << "\n";

		// Extra payload preview (up to first 64 bytes)
		if (m->ExtraDataPtrEnabled && m->ExtraDataPtr && msg.size() >= (sizeof(icsSpyMessageVSB) + (size_t)m->ExtraDataPtr))
		{
			const uint8_t* payload = &msg[sizeof(icsSpyMessageVSB)];
			size_t plen = (size_t)m->ExtraDataPtr;
			size_t sample = plen < 64 ? plen : 64;
			out << "ExtraData (first " << sample << "/" << plen << " bytes): ";
			bytesHex(payload, sample);
			out << "\n";
		}

		out << "\n";
	}

	out.flush();
	return true;
}

