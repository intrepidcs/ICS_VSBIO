#include "PcapFile.h"
#include "VSBIO.h"

struct pcap_file_header
{
	uint32_t magic_number; /* magic number */
	uint16_t version_major; /* major version number */
	uint16_t version_minor; /* minor version number */
	int32_t  thiszone; /* GMT to local correction */
	uint32_t sigfigs; /* accuracy of timestamps */
	uint32_t snaplen; /* max length of captured packets, in octets */
	uint32_t network; /* data link type */
	pcap_file_header() : magic_number(0xa1b23c4d), // Regular ordering, nanosecond precision
		version_major(2), version_minor(4), thiszone(0), sigfigs(0), snaplen(65535), network(1) {}
};

struct pcaprec_hdr
{
	uint32_t ts_sec; /* timestamp seconds */
	uint32_t ts_usec; /* timestamp microseconds */
	uint32_t incl_len; /* number of octets of packet saved in file */
	uint32_t orig_len; /* actual length of packet */
	pcaprec_hdr() : ts_sec(0), ts_usec(0), incl_len(0), orig_len(0) {}
};

struct PcapNgBlock
{
	uint32_t m_uiBlockType;
	uint32_t m_uiLength;
	PcapNgBlock(uint32_t type = 0, uint32_t len = 0) : m_uiBlockType(type), m_uiLength(len) {}
};

struct SectionHeaderBlock
{
	uint32_t m_uiByteOrderMagic;
	uint16_t m_uiMajorVersion;
	uint16_t m_uiMinorVersion;
	int64_t m_uiSectionLength;
	SectionHeaderBlock() : m_uiByteOrderMagic(0x1a2b3c4d), m_uiMajorVersion(1),	m_uiMinorVersion(0), m_uiSectionLength(-1) {}
};

struct InterfaceDescriptionBlock
{
	uint16_t m_uiLinkType;
	uint16_t m_uiReserved;
	uint32_t m_uiSnapLen;
	InterfaceDescriptionBlock() : m_uiLinkType(1), m_uiReserved(0), m_uiSnapLen(0xFFFFFFFF) {}
};

struct EnhancedPacketBlock
{
	uint32_t m_uiInterfaceID;
	uint32_t m_uiTimestampHigh;
	uint32_t m_uiTimestampLow;
	uint32_t m_uiCapturedLen;
	uint32_t m_uiPacketLen;
	EnhancedPacketBlock() : m_uiInterfaceID(0), m_uiTimestampHigh(0), m_uiTimestampLow(0), m_uiCapturedLen(0), m_uiPacketLen(0) {}
};

bool PcapFile::Open(const char* pFilePath, bool append)
{
	file = fopen(pFilePath, append ? "ab" : "wb");
	if (file && !append)
	{
		if (type == PcapType::pcapType)
		{
			pcap_file_header header;
			fwrite(&header, sizeof(pcap_file_header), 1, file);
		}
		else
		{
			PcapNgBlock initblock(0x0A0D0D0A, sizeof(PcapNgBlock) + sizeof(SectionHeaderBlock) + 4);
			fwrite(&initblock, sizeof(initblock), 1, file);

			SectionHeaderBlock obSHB;
			fwrite(&obSHB, sizeof(SectionHeaderBlock), 1, file);
			fwrite(&initblock.m_uiLength, 4, 1, file);

			initblock.m_uiBlockType = 0x1;
			initblock.m_uiLength = sizeof(initblock) + sizeof(InterfaceDescriptionBlock) + 4;
			fwrite(&initblock, sizeof(initblock), 1, file);

			InterfaceDescriptionBlock obIDB;
			fwrite(&obIDB, sizeof(InterfaceDescriptionBlock), 1, file);
			fwrite(&initblock.m_uiLength, 4, 1, file);
		}
	}
	return (file != nullptr);
}

bool PcapFile::WriteMessage(const std::vector<unsigned char>& data)
{
	if (file == nullptr)
		return false;

	icsSpyMessageVSB* msg = ((icsSpyMessageVSB*)&data[0]);
	if ((msg->Protocol == SPY_PROTOCOL_ETHERNET) && (data.size() > sizeof(icsSpyMessageVSB)))
	{
		uint64_t timestamp = CMessageTimeDecoderVSB::CalcEpoch64(*msg);
		uint32_t payloadSize = (uint32_t)(data.size() - sizeof(icsSpyMessageVSB));
		uint32_t pad = 0;
		++numRecords;
		if (type == PcapType::pcapType)
		{
			pcaprec_hdr recHeader;
			recHeader.ts_sec = (uint32_t)(timestamp / 1000000000ull);
			recHeader.ts_usec = (uint32_t)(timestamp - (recHeader.ts_sec * 1000000000));
			recHeader.incl_len = recHeader.orig_len = payloadSize;
			fwrite(&recHeader, sizeof(recHeader), 1, file);
			fwrite(&data[sizeof(icsSpyMessageVSB)], payloadSize, 1, file);
		}
		else
		{
			EnhancedPacketBlock epb;
			epb.m_uiCapturedLen = epb.m_uiPacketLen = payloadSize;
			epb.m_uiTimestampLow = (uint32_t)(timestamp / 1000);
			epb.m_uiTimestampHigh = (timestamp / 1000) >> 32;
			uint32_t padding = payloadSize % 4;
			if (padding > 0)
				padding = (4 - padding);
			PcapNgBlock initblock(0x6, payloadSize + sizeof(PcapNgBlock) + sizeof(EnhancedPacketBlock) + 4 + padding);
			fwrite(&initblock, sizeof(initblock), 1, file);
			fwrite(&epb, sizeof(EnhancedPacketBlock), 1, file);
			fwrite(&data[sizeof(icsSpyMessageVSB)], payloadSize, 1, file);
			if (padding)
				fwrite(&pad, padding, 1, file);
			fwrite(&initblock.m_uiLength, 4, 1, file);
		}
		return true;
	}
	return false;
}

bool PcapFile::Close()
{
	if (file)
		return (fclose(file) == 0);
	return true;
}
