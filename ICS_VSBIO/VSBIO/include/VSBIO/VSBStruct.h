#ifndef _VSBStruct
#define _VSBStruct

#ifdef _WIN32
// Visual studio has extremely poor support for C99 pre-2010
typedef signed char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef __int64 int64_t;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned __int64 uint64_t;
#else
#include <inttypes.h>
#include <stddef.h>
#endif

template <typename Type, size_t N>
struct wrapped_array {
	Type data[N];
};

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
	wrapped_array<uint8_t, 8> Data;
	union {
		struct
		{
			uint32_t StatusBitField3;
			uint32_t StatusBitField4;
		};
		wrapped_array<uint8_t, 8> AckBytes;
	};
	uint32_t ExtraDataPtr;
	uint8_t MiscData;
	wrapped_array<uint8_t, 3> Reserved;
} icsSpyMessageVSB;
#define icsSpyMessageVSB_SIZE 64

#endif