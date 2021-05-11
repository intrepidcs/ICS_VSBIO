//---------------------------------------------------------------------------

#ifndef MessageTimeDecoderVSBH
#define MessageTimeDecoderVSBH
//---------------------------------------------------------------------------
#ifndef MessageTimeDecoder_h
#define MessageTimeDecoder_h

#include "VSBStruct.h"
#include "VSBFlags.h"

class CMessageTimeDecoderVSB
{
public:
	typedef double secs_t;
	static secs_t CalcTimeStamp(const icsSpyMessageVSB& obMsg);
	static uint64_t CalcEpoch64(const icsSpyMessageVSB& obMsg);
	static void SetMessageTime(icsSpyMessageVSB& msg, uint64_t timeVal);

private:
	static int GetSpyTimeType(const icsSpyMessageVSB& msg);

};

#endif// MessageTimeDecoder_h
#endif
