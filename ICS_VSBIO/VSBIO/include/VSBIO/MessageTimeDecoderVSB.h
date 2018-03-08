//---------------------------------------------------------------------------

#ifndef MessageTimeDecoderVSBH
#define MessageTimeDecoderVSBH
//---------------------------------------------------------------------------
#ifndef MessageTimeDecoder_h
#define MessageTimeDecoder_h

#include "VSBIO/VSBStruct.h"
#include "VSBIO/VSBFlags.h"

class CMessageTimeDecoderVSB
{
public:
	typedef double secs_t;
	static secs_t CalcTimeStamp(const icsSpyMessageVSB& obMsg);

private:
	static int GetSpyTimeType(const icsSpyMessageVSB& msg);

};

#endif// MessageTimeDecoder_h
#endif
