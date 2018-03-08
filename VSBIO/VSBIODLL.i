/*VSBIODLL.i*/
%module VSBIOInterface
%{
	#include "VSBIODLL.h"
%}

%typemap(in,numinputs=0,noblock=1) size_t *len  {
  size_t templen;
  $1 = &templen;
}

%typemap(out) char* GetEDP {
  int i;
  $result = PyByteArray_FromStringAndSize($1, templen);
}

typedef signed char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef __int64 int64_t;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned __int64 uint64_t;

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

%apply int *OUTPUT {unsigned int * lengthOfMessageReturned};

%include "VSBIODLL.h"

%inline %{

	typedef	enum  _VSBRtnValues{
		eSuccess = 0,
		eEndOfFile = 1,
		eError = 2,
		eBufferToSmall = 3
	} VSBRtnValues;

	char * GetEDP(icsSpyMessageVSB * message, size_t *len)
	{
		char * edpreturn =  ((char *)(message) + sizeof(icsSpyMessageVSB));
		*len = ((icsSpyMessageVSB *)message)->ExtraDataPtr ;
		return edpreturn;
	}
%}






