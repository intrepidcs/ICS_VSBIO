//---------------------------------------------------------------------------


#include "MessageTimeDecoderVSB.h"
//---------------------------------------------------------------------------

#include "VSBStruct.h"
#include "VSBFlags.h"
#include <cstring> //for memcpy


typedef CMessageTimeDecoderVSB::secs_t secs_t;

/// <summary>
/// Computes the UTC timestamp for the given message
/// </summary>
/// <param name="obMsg">Message to process</param>
/// <returns>Number of seconds since midnight Jan 1, 2007</returns>
secs_t CMessageTimeDecoderVSB::CalcTimeStamp(const icsSpyMessageVSB& obMsg)
{
	int iTimeStampID = GetSpyTimeType(obMsg);
	secs_t retval = 0;

	// calculate the timestamp
	if (iTimeStampID == HARDWARE_TIMESTAMP_ID_NEORED_10NS)
	{
		retval = ((double)((unsigned int)obMsg.TimeHardware2)) * NEOVI_RED_TIMESTAMP_2_10NS +
			((double)((unsigned int)obMsg.TimeHardware)) * NEOVI_RED_TIMESTAMP_1_10NS;
	}
	else if (iTimeStampID == HARDWARE_TIMESTAMP_ID_NEORED_25NS)
	{
		retval = ((double)((unsigned int)obMsg.TimeHardware2)) * NEOVI_RED_TIMESTAMP_2_25NS +
			((double)((unsigned int)obMsg.TimeHardware)) * NEOVI_RED_TIMESTAMP_1_25NS;
	}
	else if (iTimeStampID == HARDWARE_TIMESTAMP_ID_NEORED_10US)
	{
		retval = ((double)((unsigned int)obMsg.TimeHardware2)) * NEOVI_RED_TIMESTAMP_2_10US +
			((double)((unsigned int)obMsg.TimeHardware)) * NEOVI_RED_TIMESTAMP_1_10US;
	}
	else if (iTimeStampID == HARDWARE_TIMESTAMP_ID_NEOv6_VCAN)
	{
		retval = ((double)obMsg.TimeHardware2) * 0.065536 + ((double)obMsg.TimeHardware) * 0.000001;
	}
	else if (iTimeStampID == HARDWARE_TIMESTAMP_ID_NEOVI)
	{
		retval = ((double)obMsg.TimeHardware2) * 0.1048576 + ((double)obMsg.TimeHardware) * 0.0000016;
	}
	else if (iTimeStampID == HARDWARE_TIMESTAMP_ID_DOUBLE_SEC)
	{
		memcpy(&retval, &obMsg.TimeHardware, sizeof(double));
	}
	else
	{
		retval = ((double)obMsg.TimeSystem) / 1000;
	}

	return retval;
}


const uint64_t NEOVI_RED_TIMESTAMP_2_25NS_UINT64 = 107374182400;
const uint64_t NEOVI_RED_TIMESTAMP_1_25NS_UINT64 = 25;

const uint64_t NEOVI_RED_TIMESTAMP_2_10NS_UINT64 = 429496729600;
const uint64_t NEOVI_RED_TIMESTAMP_1_10NS_UINT64 = 10;

const uint64_t NEOVI_RED_TIMESTAMP_2_10US_UINT64 = 42949672960000;
const uint64_t NEOVI_RED_TIMESTAMP_1_10US_UINT64 = 10000;

const uint64_t NEOVIPRO_VCAN_TIMESTAMP_2_US_UINT64 = 65536000;
const uint64_t NEOVIPRO_VCAN_TIMESTAMP_1_US_UINT64 = 1000;

const unsigned char SYSTEM_TIMESTAMP_ID_NONE = 0;
const unsigned char SYSTEM_TIMESTAMP_ID_TIMEGETTIME_API = 1;

/// <summary>
/// Computes the time64_t for the given message
/// </summary>
/// <param name="obMsg">Message to process</param>
/// <returns>Epoch value: number of nanoseconds since Jan 1, 1970</returns>
uint64_t CMessageTimeDecoderVSB::CalcEpoch64(const icsSpyMessageVSB& obMsg)
{
	// calculate the timestamp
	switch (GetSpyTimeType(obMsg))
	{
		case HARDWARE_TIMESTAMP_ID_NEORED_10NS:
		return (uint64_t)obMsg.TimeHardware2 * NEOVI_RED_TIMESTAMP_2_10NS_UINT64 +
			(uint64_t)obMsg.TimeHardware * NEOVI_RED_TIMESTAMP_1_10NS_UINT64 + ICS_EPOCH_OFFSET;

		case HARDWARE_TIMESTAMP_ID_NEORED_25NS:
		return (uint64_t)obMsg.TimeHardware2 * NEOVI_RED_TIMESTAMP_2_25NS_UINT64 +
			(uint64_t)obMsg.TimeHardware * NEOVI_RED_TIMESTAMP_1_25NS_UINT64 + ICS_EPOCH_OFFSET;

		case HARDWARE_TIMESTAMP_ID_NEORED_10US:
		return (uint64_t)obMsg.TimeHardware2 * NEOVI_RED_TIMESTAMP_2_10US_UINT64 +
			(uint64_t)obMsg.TimeHardware * NEOVI_RED_TIMESTAMP_1_10US_UINT64 + ICS_EPOCH_OFFSET;

		case HARDWARE_TIMESTAMP_ID_NEOv6_VCAN:
		return (uint64_t)obMsg.TimeHardware2 * NEOVIPRO_VCAN_TIMESTAMP_2_US_UINT64 + 
			(uint64_t)obMsg.TimeHardware * NEOVIPRO_VCAN_TIMESTAMP_1_US_UINT64 + ICS_EPOCH_OFFSET;

		case HARDWARE_TIMESTAMP_ID_NEOVI:
		return (uint64_t)obMsg.TimeHardware2 * 104857600 + (uint64_t)obMsg.TimeHardware * 1600 + ICS_EPOCH_OFFSET;

		case HARDWARE_TIMESTAMP_ID_DOUBLE_SEC:
		{
			double dSec;
			memcpy(&dSec, &obMsg.TimeHardware, sizeof(double));
			return (uint64_t)(dSec * 1e9);
		}
		
		default:
		return (uint64_t)obMsg.TimeSystem * 1000000;
	}
}

/// <summary>
/// Sets the message time from the time64_t value
/// </summary>
/// <param name="msg">Message to update</param>
/// <param name="timeVal">Epoch value: number of nanoseconds since Jan 1, 1970</param>
void CMessageTimeDecoderVSB::SetMessageTime(icsSpyMessageVSB& msg, uint64_t timeVal)
{
	msg.TimeStampHardwareID = HARDWARE_TIMESTAMP_ID_NEORED_25NS;
	timeVal -= ICS_EPOCH_OFFSET;
	timeVal /= 25;
	msg.TimeHardware = (uint32_t)timeVal;
	msg.TimeHardware2 = (uint32_t)(timeVal >> 32);
}

int CMessageTimeDecoderVSB::GetSpyTimeType(const icsSpyMessageVSB& msg)
{
	return (msg.TimeStampHardwareID & 0x80) ? -1 : (msg.TimeStampHardwareID & 0x7F);
}
