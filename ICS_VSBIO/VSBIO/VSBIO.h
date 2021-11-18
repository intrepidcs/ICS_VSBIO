// ---------------------------------------------------------------------------

#ifndef VSBIOH
#define VSBIOH
// ---------------------------------------------------------------------------

#include <vector>
#include <string>
#include "OFile.h"
#include <queue>
#include "MessageTimeDecoderVSB.h"

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
		eBufferTooSmall = 3,
		eInvalidPayloadSize = 4
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
	std::string mFileName;
	std::string mFullFileName;

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

	VSBIORead(const std::string& sFileNames);
	~VSBIORead();
	enumFileCondition ReadNextMessage(unsigned char * message, size_t sizeOfBuffer, size_t * returnLength); //used by DLL
	enumFileCondition ReadNextMessage(std::vector<unsigned char>& message); //the size of the vector indicates the size of the message.

	int GetProgress();
	std::string GetDisplayMessage() { return mDisplayOut; }
	std::string GetErrorMessage() { return mErrorOut; }
	void UpdateUIOutput(VSBIO & vsbio);

    bool Split(const uint64_t& nMessagesPerFile, const std::string& OutputLocation, ProgressFunc prog);

protected:
	std::string mErrorOut;
	std::string mDisplayOut;
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

	std::string mDisplayOut, mErrorOut;
	std::vector<unsigned char> vecMessage;
	unsigned char* message;
	bool mInitError;
public:
	VSBIOReadMultiple(const std::vector<std::string>& sFileNames);
	~VSBIOReadMultiple();
	VSBIORead::enumFileCondition ReadNextMessage(unsigned char * message, size_t sizeOfBuffer, size_t * returnLength); //used by DLL
	VSBIORead::enumFileCondition ReadNextMessage(std::vector<unsigned char>& message); //the size of the vector indicates the size of the message.

	int GetProgress();
	std::string GetDisplayMessage() { return mDisplayOut; }
	std::string GetErrorMessage() { return mErrorOut; }
	void UpdateUIOutput(VSBIO &vsbio);
};

class VSBIOWrite
{
private:
	std::string mFileName;
	OFile mCurrentFP;
	bool mFileOpen;

public:
	VSBIOWrite();	
	~VSBIOWrite();
	bool Init(const std::string& fileName);
	bool WriteMessage(const unsigned char *  message, const unsigned int& size);
	bool WriteMessage(const std::vector<unsigned char>& message);
	std::string GetDisplayMessage() { return DisplayOut; }
	bool FileOpen(){return mFileOpen;}
    bool Concatenate(std::vector<std::string> &sInputFileList, ProgressFunc prog);
    bool ConcatenateFromDirectory(const std::string &sInputFilePath, ProgressFunc prog);
protected:
	std::string DisplayOut;
};

class VSBIO
{
public:
	std::string GetError() { return mErrorOut; }
	std::string GetOutput() { return mOutput; }
	virtual bool IsRunning() { return mIsRunning; }
	VSBIO() : mIsRunning(true), mOutput(""), mErrorOut(""), mProgress(0) {}
	virtual std::string GetType() = 0;
protected:

#ifdef VSBIOCMD
	void AppendOutput(std::string text) {}
	void AppendError(std::string text) {}
#else
	void AppendOutput(std::string text) {mOutput += text; }
	void AppendError(std::string text) {mOutput += text; mErrorOut += text; }
#endif
	void SetRunning(bool running) {mIsRunning = running; }

 private:
	bool mIsRunning;
	std::string mOutput;
	std::string mErrorOut;
    int mProgress;

	friend void VSBIORead::UpdateUIOutput(VSBIO & vsbio);
	friend void VSBIOReadMultiple::UpdateUIOutput(VSBIO & vsbio);
};


#endif
