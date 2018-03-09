// ---------------------------------------------------------------------------

#ifndef VSBIOH
#define VSBIOH
// ---------------------------------------------------------------------------

#include <vector>
#include <string>
#include "StandardLibrary/OFile.h"
#include <queue>
#include "VSBIO/MessageTimeDecoderVSB.h"

#define NUM_MSGS_TO_READ 500000
#define MSG_BUFFER_SIZE 50000//arbitrary number of messages

using namespace std;

struct VSBSpyMsgTime
{
	unsigned long SystemTimeStampID;// 4
	unsigned long SystemTime1;// 4
	unsigned long SystemTime2;// 4
	unsigned long HardwareTimeStampID;// 4
	unsigned long HardwareTime1;// 4
	unsigned long HardwareTime2;// 4
};

typedef icsSpyMessageVSB VSBSpyMessage;
struct VSBSpyMessageTimeSort
{
	bool operator()(const VSBSpyMessage*& a, const VSBSpyMessage*& b) const
	{
		return  CMessageTimeDecoderVSB::CalcTimeStamp(*a) -  CMessageTimeDecoderVSB::CalcTimeStamp(*b) >= 0;
	}
};
class VSBIO;
class VSBIOReadMultiple;
class VSBIORead
{
public:
	enum enumFileCondition
	{//please dont change the values of these enums, dlls use these as int
		eSuccess = 0,
		eEndOfFile = 1,
		eError = 2,
		eBufferToSmall = 3
	};

private:
	enum enumVSBIOVersion
	{
		VSBIONone,
		VSBIO101,
		VSBIO102,
		VSBIO103,
		VSBIO104,
	};
	std::wstring mFileName;
	std::wstring mFullFileName;

	std::vector<unsigned char> vecMessage;
	unsigned char * message;
	size_t messageBufferSize;
	
	OFile mCurrentFile;

	enumVSBIOVersion mCurrentFileType;
	unsigned long long mEDPStartLocation, mCurrentEDPLocation, mMsgStartLocation, mCurrentMsgLocation;
	unsigned long long mCurrentFileSize, mBufferSizeRequired;
	unsigned int mCurrentEDPIndex;

	bool Init101();
	bool Init102();
	bool Init103();
	bool Init104();

	enumFileCondition Read101();
	enumFileCondition Read102();
	enumFileCondition Read103();
	enumFileCondition Read104();

	enumFileCondition GetEDP(unsigned int requestedEDP);

	// the following three functions are used by ReadMultiple
//********************************************************
	enumFileCondition ReadNextMessage();
	unsigned char * GetMessage() { return message; }
	size_t GetMessageSize() const
	{
		return (mCurrentFileType == VSBIONone) ? 0 : sizeof(VSBSpyMessage) + ((VSBSpyMessage*)message)->ExtraDataPtr;
	}
//********************************************************
	friend VSBIOReadMultiple;
public:

	VSBIORead(const std::wstring& sFileNames);
	~VSBIORead();
	enumFileCondition ReadNextMessage(unsigned char * message, size_t sizeOfBuffer, size_t * returnLength); //used by DLL
	enumFileCondition ReadNextMessage(std::vector<unsigned char>& message); //the size of the vector indicates the size of the message.

	int GetProgress();
	std::wstring GetDisplayMessage() { return mDisplayOut; }
	std::wstring GetErrorMessage() { return mErrorOut; }
	void UpdateUIOutput(VSBIO & vsbio);

protected:
	std::wstring mErrorOut;
	std::wstring mDisplayOut;
};

class VSBIOReadMultiple
{
	struct cmp
	{
		bool operator()(const std::pair<VSBSpyMessage*, VSBIORead*>& a, const std::pair<VSBSpyMessage*, VSBIORead*>& b) const
		{
			return CMessageTimeDecoderVSB::CalcTimeStamp(*(a.first)) - CMessageTimeDecoderVSB::CalcTimeStamp(*(b.first)) >= 0;
		}
	};

	typedef std::vector<VSBIORead*> ReadList;
	ReadList IOReadList;
	typedef std::priority_queue<std::pair<VSBSpyMessage*, VSBIORead*>, vector<std::pair<VSBSpyMessage*, VSBIORead*> >, cmp> PriorityQueue;
	PriorityQueue latestTimestamp;

	std::wstring mDisplayOut, mErrorOut;
	std::vector<unsigned char> vecMessage;
	unsigned char* message;
	bool mInitError;
public:
	VSBIOReadMultiple(const std::vector<std::wstring>& sFileNames);
	~VSBIOReadMultiple();
	VSBIORead::enumFileCondition ReadNextMessage(unsigned char * message, size_t sizeOfBuffer, size_t * returnLength); //used by DLL
	VSBIORead::enumFileCondition ReadNextMessage(std::vector<unsigned char>& message); //the size of the vector indicates the size of the message.

	int GetProgress();
	std::wstring GetDisplayMessage() { return mDisplayOut; }
	std::wstring GetErrorMessage() { return mErrorOut; }
	void UpdateUIOutput(VSBIO &vsbio);
};

class VSBIOWrite
{
private:
	std::wstring mFileName;
	OFile mCurrentFP;
	bool mFileOpen;

public:
	VSBIOWrite();	
	~VSBIOWrite();
	bool Init(const std::wstring& fileName);
	bool WriteMessage(const unsigned char *  message, const unsigned int& size);
	bool WriteMessage(const std::vector<unsigned char>& message);
	std::wstring GetDisplayMessage() { return DisplayOut; }
	bool FileOpen(){return mFileOpen;}
protected:
	std::wstring DisplayOut;
};

class VSBIO
{
public:
	std::wstring GetError() { return mErrorOut; }
	std::wstring GetOutput() { return mOutput; }
	virtual bool IsRunning() { return mIsRunning; }
	VSBIO() : mIsRunning(true), mOutput(L""), mErrorOut(L"") {}
	virtual std::wstring GetType() = 0;
protected:

#ifdef VSBIOCMD
#ifdef _WIN32
	void AppendOutput(std::wstring text) {wprintf(L"%s", text.c_str());}
	void AppendError(std::wstring text) {fwprintf(stderr, L"%s", text.c_str());}
#else
	void AppendOutput(std::wstring text) {}
	void AppendError(std::wstring text) {}
#endif
#else
	void AppendOutput(std::wstring text) {mOutput += text; }
	void AppendError(std::wstring text) {mOutput += text; mErrorOut += text; }
#endif
	void SetRunning(bool running) {mIsRunning = running; }


 private:
	bool mIsRunning;
	std::wstring mOutput;
	std::wstring mErrorOut;

	friend void VSBIORead::UpdateUIOutput(VSBIO & vsbio);
	friend void VSBIOReadMultiple::UpdateUIOutput(VSBIO & vsbio);
};


#endif
