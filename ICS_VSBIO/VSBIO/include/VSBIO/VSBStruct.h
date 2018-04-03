#ifndef _VSBStruct
#define _VSBStruct

#include <inttypes.h>

typedef struct _icsSpyMessageVSB
{
	uint32_t StatusBitField;
	uint32_t StatusBitField2;
	uint32_t TimeHardware;
	uint32_t TimeHardware2;
	uint32_t TimeSystem;
	uint32_t TimeSystem2;
	uint8_t TimeStampHardwareID;
	uint8_t TimeStampSystemID;
	uint8_t NetworkID;
	uint8_t NodeID;
	uint8_t Protocol;
	uint8_t MessagePieceID;
	uint8_t ExtraDataPtrEnabled;
	uint8_t NumberBytesHeader;
	uint8_t NumberBytesData;
	uint8_t NetworkID2;
	int16_t DescriptionID;
	uint32_t ArbIDOrHeader;
	uint8_t Data[8];
	union {
		struct
		{
			uint32_t StatusBitField3;
			uint32_t StatusBitField4;
		};
		uint8_t AckBytes[8];
	};
	uint32_t ExtraDataPtr;
	uint8_t MiscData;
	uint8_t Reserved[3];
} icsSpyMessageVSB;
#define icsSpyMessageVSB_SIZE 64

#endif