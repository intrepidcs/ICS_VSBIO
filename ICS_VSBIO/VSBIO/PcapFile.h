#pragma once

#include <stdint.h>
#include <vector>

enum class PcapType
{
	pcapType,
	pcapngType
};

class PcapFile
{
	uint64_t numRecords;
	PcapType type;
	FILE* file;
public:

	PcapFile(PcapType inType) : numRecords(0), type(inType), file(nullptr) {}
	~PcapFile() { Close(); }

	/// <summary>
	/// Opens the pcap/pcapng file requested
	/// </summary>
	/// <param name="pFilePath">Path of file to open</param>
	/// <param name="append">Append to file</param>
	/// <returns>true if successful</returns>
	bool Open(const char* pFilePath, bool append);

	/// <summary>
	/// Writes an Ethernet message to the previously opened file
	/// </summary>
	/// <param name="msg">Message to write</param>
	/// <returns>true if record was written</returns>
	bool WriteMessage(const std::vector<unsigned char>& msg);

	/// <summary>
	/// Closes the opened file
	/// </summary>
	/// <returns></returns>
	bool Close();

	uint64_t GetRecordsWritten() const { return numRecords; }
};