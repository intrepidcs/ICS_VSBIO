//---------------------------------------------------------------------------


#include "VSBIO/MessageTimeDecoderVSB.h"
//---------------------------------------------------------------------------

#include "VSBIO/VSBStruct.h"
#include "VSBIO/VSBFlags.h"
#include <cstring> //for memcpy


typedef CMessageTimeDecoderVSB::secs_t secs_t;

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

	return retval;
}


int CMessageTimeDecoderVSB::GetSpyTimeType(const icsSpyMessageVSB& msg)
{
	return (msg.TimeStampHardwareID & 0x80)?-1.0:msg.TimeStampHardwareID & 0x7F;
}