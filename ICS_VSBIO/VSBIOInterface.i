/*VSBIODLL.i*/
%module VSBIOInterface
%{
	#include "VSBIODLL.h"
%}
%feature("flatnested");
%typemap(in,numinputs=0,noblock=1) size_t *len  {
  size_t templen;
  $1 = &templen;
}

%typemap(out) char* GetEDP {
  int i;
  $result = PyByteArray_FromStringAndSize($1, templen);
}

%apply int *OUTPUT {unsigned int * lengthOfMessageReturned};

%include "VSBIODLL.h"
%include "VSBIO/include/VSBIO/VSBStruct.h"

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






