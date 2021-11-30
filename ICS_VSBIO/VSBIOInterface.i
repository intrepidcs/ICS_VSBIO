/*VSBIODLL.i*/
%include "stdint.i"
%module VSBIOInterface
%include "std_wstring.i"
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
%include "VSBIO/VSBStruct.h"

%inline %{

	typedef	enum  _VSBRtnValues{
		eSuccess = 0,
		eEndOfFile = 1,
		eError = 2,
		eBufferTooSmall = 3,
		eInvalidPayloadSize = 4
	} VSBRtnValues;

	char * GetEDP(icsSpyMessageVSB * message, size_t *len)
	{
		char * edpreturn =  ((char *)(message) + sizeof(icsSpyMessageVSB));
		*len = ((icsSpyMessageVSB *)message)->ExtraDataPtr ;
		return edpreturn;
	}

  unsigned char GetByteFromData(icsSpyMessageVSB * message, size_t index)
  {
    if (index > 8)
      return 0;
    else
      return (unsigned char)(message->Data.data[index]);
  }


%}

%include "std_except.i"

%extend wrapped_array {
  inline size_t __len__() const { return N; }

  inline const Type& __getitem__(size_t i) const throw(std::out_of_range) {
    if (i >= N || i < 0)
      throw std::out_of_range("out of bounds access");
    return self->data[i];
  }

  inline void __setitem__(size_t i, const Type& v) throw(std::out_of_range) {
    if (i >= N || i < 0)
      throw std::out_of_range("out of bounds access");
    self->data[i] = v;
  }
}

%template (intArray8) wrapped_array<uint8_t, 8>;
%template (intArray3) wrapped_array<uint8_t, 3>;



