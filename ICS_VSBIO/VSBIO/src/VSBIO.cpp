// ---------------------------------------------------------------------------

#pragma hdrstop

#include "VSBIO/VSBIO.h"
#ifndef linux
#include <direct.h>
#define GetCurrentDir _getcwd
#define DASH L"\\"
#else
#include <unistd.h>
#define GetCurrentDir getcwd
#define DASH L"/"
#endif

#include <stdio.h>
#include <string.h>
#include <limits>

#define VSB_HEADERSIZE 10
#define VSB_2_3_EDPHEADERSIZE 16

// ---------------------------------------------------------------------------
#pragma package(smart_init)

VSBIORead::VSBIORead(const std::wstring& sFileNames) : mFullFileName(sFileNames)
{
	mCurrentMsgLocation = 0;
	mCurrentFileSize = 50000;// number over 64 so the first message does not closefile
	mDisplayOut = L"";
	int npos = mFullFileName.rfind(DASH) + std::wstring(DASH).length();
	mFileName = mFullFileName.substr(npos, mFullFileName.length() - npos);
	mCurrentFileType = VSBIONone;
}

void VSBIORead::UpdateUIOutput(VSBIO & vsbio)
{
	vsbio.AppendOutput(mDisplayOut);
	vsbio.AppendError(mErrorOut);
}

VSBIORead::~VSBIORead()
{
	mCurrentFile.CloseFile();
}

int VSBIORead::GetProgress()
{
	return (float)(mCurrentMsgLocation - mMsgStartLocation) / (float)(mCurrentFileSize - mMsgStartLocation) * 100;
}


VSBIORead::enumFileCondition VSBIORead::ReadNextMessage()
{
	return ReadNextMessage(vecMessage);
} 

VSBIORead::enumFileCondition VSBIORead::ReadNextMessage(std::vector<unsigned char>& message)
{
	enumFileCondition returnvalue;
	if (message.size() <  sizeof(VSBSpyMessage))
		message.resize(sizeof(VSBSpyMessage));
	size_t retval;
	while ((returnvalue = ReadNextMessage(&message[0], message.size(), &retval)) == eBufferToSmall)	
		message.resize(retval);	
	message.resize(retval); //should always make the vector smaller
	return returnvalue;
}

VSBIORead::enumFileCondition VSBIORead::ReadNextMessage(unsigned char * message, size_t messageBufferSize, size_t * returnLength )
{
	this->message = message;
	this->messageBufferSize = messageBufferSize;
	*returnLength = mBufferSizeRequired = sizeof(VSBSpyMessage);
	if (mBufferSizeRequired < sizeof(VSBSpyMessage))
		return eBufferToSmall;

	unsigned long read;
	bool error = false;
	mDisplayOut = L"";
	if (mCurrentMsgLocation + sizeof(VSBSpyMsgTime) >= mCurrentFileSize)
	{
		mDisplayOut += mFileName + L": Closed\n";
		mCurrentFile.CloseFile();//we ignore the time stamp at the end of 0x103 since 0x104 does not have it
		return eEndOfFile;
	}

	if (mCurrentFileType == VSBIONone)
	{
		if (!mCurrentFile.OpenFile(mFullFileName.c_str(), false, false))
		{
			mErrorOut += mFileName + L": Could Not Open\n";
			mCurrentFile.CloseFile();
			return eError;
		}

		char id[7];
		mCurrentFile.Read(id, 6, read);
		if ((read != 6) || (memcmp(id, "icsbin", 6) != 0))
		{
			mErrorOut += mFileName + L": Invalid vsb file\n";
			mCurrentFile.CloseFile();
			return eError;
		}
		unsigned int fileversion;

		mCurrentFile.Read(&fileversion, 4, read);

		if (fileversion == 0x101)
			Init101();
		else if (fileversion == 0x102)
			Init102();
		else if (fileversion == 0x103)
			Init103();
		else if (fileversion == 0x104)
			Init104();
		else
		{
			mErrorOut += mFileName + L": Invalid version\n";
			mCurrentFile.CloseFile();
			return eError;
		}
		mDisplayOut += mFileName + L": Opened\n";
		mCurrentFileSize = mCurrentFile.FileSizeLarge();
	}

	enumFileCondition condition;
	switch (mCurrentFileType)
	{
		case VSBIO101:
			condition = Read101();
			break;
		case VSBIO102:
			condition = Read102();
			break;
		case VSBIO103:
			condition = Read103();
			break;
		case VSBIO104:
			condition = Read104();
			break;
	}
	*returnLength = mBufferSizeRequired;
	return condition;
}

bool VSBIORead::Init101()
{
	mCurrentFileType = VSBIO101;
	unsigned int vs3size;
	unsigned long read;
	mCurrentFile.Read(&vs3size, 4, read);
	mCurrentFile.SetFilePtr(vs3size, SEEK_CUR);
	mMsgStartLocation = mCurrentMsgLocation = mCurrentFile.GetFilePtrLong();
	return true;
}

bool VSBIORead::Init102()
{
	mCurrentFileType = VSBIO102;
	static const char* EDP_SECTION = "EDP_SECTION";
	static unsigned int EDP_SECTION_LEN = strlen(EDP_SECTION);

	mCurrentEDPIndex = 1;
	unsigned long read;
	unsigned int EDPSize;
	unsigned long locationBefore = mCurrentFile.GetFilePtrLong();
	mCurrentFile.Read(&EDPSize, 4, read);
	if (EDPSize > 0)
	{
		// resize the setup array to fit the setup file
		std::vector<unsigned char> obSetupArray(EDP_SECTION_LEN + 1);

		// step 3: read the setup file ///////////////////////////////////////////////////////////////////////
		mCurrentFile.Read(&obSetupArray[0], EDP_SECTION_LEN + 1, read);

		// check if there's an EDP_SECTION in here
		if (EDPSize >= EDP_SECTION_LEN + 1 && strcmp((const char*)&obSetupArray[0], EDP_SECTION) == 0)
		{
			mEDPStartLocation = mCurrentEDPLocation = mCurrentFile.GetFilePtrLong();
			mMsgStartLocation = mCurrentMsgLocation = mCurrentFile.SetFilePtr(EDPSize - (EDP_SECTION_LEN + 1), SEEK_CUR);
		}
		else
			mMsgStartLocation = mCurrentMsgLocation = mCurrentFile.SetFilePtr(locationBefore, SEEK_SET);
	}
	else
		mMsgStartLocation = mCurrentMsgLocation = mCurrentFile.GetFilePtrLong();

	return true;
}

bool VSBIORead::Init103()
{
	Init102();
	mCurrentFileType = VSBIO103;

	unsigned long read;
	unsigned int commentlength;
	mCurrentFile.Read(&commentlength, 4, read);
	if (commentlength > 0)
		mCurrentFile.SetFilePtr((unsigned long long)commentlength * 2, SEEK_CUR);
	mCurrentFile.SetFilePtr(16, SEEK_CUR);
	mMsgStartLocation = mCurrentMsgLocation += 20 + (commentlength * 2);
	return true;
}

bool VSBIORead::Init104()
{
	mCurrentFileType = VSBIO104;
	mMsgStartLocation = mCurrentMsgLocation = mCurrentFile.GetFilePtrLong();
	return true;
}

VSBIORead::enumFileCondition VSBIORead::Read101()
{
	unsigned long read;
	mCurrentFile.Read(message, sizeof(VSBSpyMessage), read);
	VSBSpyMessage* pMessage = reinterpret_cast<VSBSpyMessage*>(message);
	pMessage->TimeStampHardwareID ^= 0x80;
	if (pMessage->TimeStampHardwareID & 0x80)
		pMessage->TimeStampHardwareID = HARDWARE_TIMESTAMP_ID_DOUBLE_SEC;
	mCurrentMsgLocation += sizeof(VSBSpyMessage);
	return eSuccess;
}

VSBIORead::enumFileCondition VSBIORead::Read102()
{
	unsigned long read;
	VSBSpyMessage* pMessageChunk = reinterpret_cast<VSBSpyMessage*>(message);

	mCurrentFile.Read(message, sizeof(VSBSpyMessage), read);
	mCurrentMsgLocation += sizeof(VSBSpyMessage);
	if (pMessageChunk->ExtraDataPtrEnabled && pMessageChunk->ExtraDataPtr)
	{
		enumFileCondition condition = GetEDP(pMessageChunk->ExtraDataPtr);
		if ( condition == eBufferToSmall)
		{
			mCurrentMsgLocation -= sizeof(VSBSpyMessage);
			mCurrentFile.SetFilePtr(mCurrentMsgLocation, SEEK_SET);
		}
		return condition;
	}
	else
	{
		pMessageChunk->ExtraDataPtrEnabled = 0;
		pMessageChunk->ExtraDataPtr = 0;
	}
	return eSuccess;
}

VSBIORead::enumFileCondition VSBIORead::Read103()
{
	enumFileCondition condition = Read102();
	if (condition != eSuccess)
		return condition;
	VSBSpyMessage* pMessageChunk = reinterpret_cast<VSBSpyMessage*>(message);
	pMessageChunk->TimeStampHardwareID ^= 0x80;
	if (pMessageChunk->TimeStampHardwareID & 0x80)
		pMessageChunk->TimeStampHardwareID = HARDWARE_TIMESTAMP_ID_DOUBLE_SEC;
	return eSuccess;
}

VSBIORead::enumFileCondition VSBIORead::Read104()
{
	unsigned long read;
	VSBSpyMessage* pMessageChunk = reinterpret_cast<VSBSpyMessage*>(message);
	mCurrentFile.Read(message, sizeof(VSBSpyMessage), read);
	mCurrentMsgLocation += sizeof(VSBSpyMessage);
	if (pMessageChunk->ExtraDataPtrEnabled && pMessageChunk->ExtraDataPtr)
	{
		mBufferSizeRequired = pMessageChunk->ExtraDataPtr + sizeof(VSBSpyMessage);
		if (pMessageChunk->ExtraDataPtr + sizeof(VSBSpyMessage) > messageBufferSize )
		{			
			mCurrentMsgLocation -= sizeof(VSBSpyMessage);
			mCurrentFile.SetFilePtr(mCurrentMsgLocation, SEEK_SET);
			return eBufferToSmall;
		}
		mCurrentMsgLocation += pMessageChunk->ExtraDataPtr;
		mCurrentFile.Read(message + sizeof(VSBSpyMessage), pMessageChunk->ExtraDataPtr, read);
	}
	else
	{
		pMessageChunk->ExtraDataPtrEnabled = 0;
		pMessageChunk->ExtraDataPtr = 0;
	}
	return eSuccess;
}

VSBIORead::enumFileCondition VSBIORead::GetEDP(unsigned int requestedEDP)
{
	VSBSpyMessage* pMessageChunk = reinterpret_cast<VSBSpyMessage*>(message);
	unsigned int EDPsize;
	unsigned long read;

	mCurrentFile.SetFilePtr(mCurrentEDPLocation, SEEK_SET);
	if (requestedEDP < mCurrentEDPIndex)
	{
		mCurrentEDPLocation = mCurrentFile.SetFilePtr(mEDPStartLocation, SEEK_SET);
		mCurrentEDPIndex = 1;
	}
	if (requestedEDP > mCurrentEDPIndex)
	{
		while (requestedEDP != mCurrentEDPIndex)
		{
			mCurrentFile.Read(&EDPsize, sizeof(EDPsize), read);
			mCurrentEDPLocation = mCurrentFile.SetFilePtr(EDPsize, SEEK_CUR);
			mCurrentEDPIndex++;
		}
	}
	
	mCurrentFile.Read(&EDPsize, sizeof(EDPsize), read);
	mBufferSizeRequired = sizeof(VSBSpyMessage) + EDPsize;
	if (sizeof(VSBSpyMessage) + EDPsize > messageBufferSize)
	{
		mCurrentFile.SetFilePtr(mCurrentEDPLocation, SEEK_SET);
		return eBufferToSmall;
	}

	pMessageChunk->ExtraDataPtr = EDPsize;
	mCurrentFile.Read(message + sizeof(VSBSpyMessage), EDPsize, read);
	mCurrentEDPLocation += EDPsize + sizeof(EDPsize);
	mCurrentEDPIndex++;
	mCurrentFile.SetFilePtr(mCurrentMsgLocation, SEEK_SET);
	return eSuccess;
}

//---------------------------------------------------------------------------------------------------------------------------

VSBIOReadMultiple::VSBIOReadMultiple(const std::vector<std::wstring> &files)
{
	for (std::vector<std::wstring>::const_iterator it = files.begin(); it != files.end(); ++it)
	{
		VSBIORead* read = new VSBIORead(*it);
		VSBIORead::enumFileCondition readState = read->ReadNextMessage();
		if (readState == VSBIORead::eSuccess)
		{
			IOReadList.push_back(read);
			mDisplayOut = read->GetDisplayMessage();
			mErrorOut = read->GetErrorMessage();
			latestTimestamp.push(std::make_pair((VSBSpyMessage*)&read->GetMessage()[0], read));
		}
		else
		{
			mDisplayOut = read->GetDisplayMessage();
			mErrorOut = read->GetErrorMessage();
			delete read;
			if (readState == VSBIORead::eError)
			{
				while (latestTimestamp.size())
				{
					delete latestTimestamp.top().second;
					latestTimestamp.pop();
				}
				mDisplayOut += L"Terminated incorectly, a Error has occurred.\n";
				mInitError = true;
			}
		}
	}
	mInitError = false;
}

VSBIOReadMultiple::~VSBIOReadMultiple()
{
	for (ReadList::iterator it = IOReadList.begin(); it != IOReadList.end(); ++it)
		delete (*it);
}

void VSBIOReadMultiple::UpdateUIOutput(VSBIO &vsbio)
{
	vsbio.AppendOutput(mDisplayOut);
	vsbio.AppendError(mErrorOut);
}

int VSBIOReadMultiple::GetProgress()
{
	int persent = 0;
	for (ReadList::iterator it = IOReadList.begin(); it != IOReadList.end(); ++it) {
		persent += (*it)->GetProgress();
	}
	return persent / IOReadList.size();
}

VSBIORead::enumFileCondition VSBIOReadMultiple::ReadNextMessage(std::vector<unsigned char> &message)
{
	VSBIORead::enumFileCondition returnvalue;
	if (message.size() <  sizeof(VSBSpyMessage))
		message.resize(sizeof(VSBSpyMessage));
	size_t retval;
	while ((returnvalue = ReadNextMessage(&message[0], message.size(), &retval)) == VSBIORead::eBufferToSmall)
		message.resize(retval);
	message.resize(retval); //should always make the vector smaller
	return returnvalue;
}

VSBIORead::enumFileCondition VSBIOReadMultiple::ReadNextMessage(unsigned char * message, size_t sizeOfBuffer, size_t * returnLength)
{
	*returnLength = 0;
	if (mInitError)
		return VSBIORead::eError;

	while (latestTimestamp.size())
	{
		std::pair<VSBSpyMessage*, VSBIORead*> pair = latestTimestamp.top();
		if (pair.second->GetMessageSize() > sizeOfBuffer)
		{
			(*returnLength) = pair.second->GetMessageSize();
			return VSBIORead::eBufferToSmall;
		}
		latestTimestamp.pop();

		memcpy(message, pair.second->GetMessage(), pair.second->GetMessageSize());
		(*returnLength) = pair.second->GetMessageSize();

		VSBIORead::enumFileCondition readState = pair.second->ReadNextMessage();
		if (readState == VSBIORead::eSuccess)
			latestTimestamp.push(std::make_pair((VSBSpyMessage*)&pair.second->GetMessage()[0], pair.second));

		mDisplayOut = pair.second->GetDisplayMessage();
		mErrorOut = pair.second->GetErrorMessage();

		if (readState == VSBIORead::eError)
		{
			while (latestTimestamp.size())
			{
				delete latestTimestamp.top().second;
				latestTimestamp.pop();
			}
			mDisplayOut += L"Terminated incorectly, a Error has occurred.";
			return readState;
		}
		return VSBIORead::eSuccess;
	}
	return VSBIORead::eEndOfFile;
}

//---------------------------------------------------------------------------------------------------------------------------

VSBIOWrite::VSBIOWrite()
{
	mFileOpen = false;
}

VSBIOWrite::~VSBIOWrite()
{
	if (mFileOpen)
		mCurrentFP.CloseFile();
}

bool VSBIOWrite::Init(const std::wstring& fileName)
{
	unsigned long read;
	mFileName = fileName;
	if (mFileOpen)
		mCurrentFP.CloseFile();
	if (!mCurrentFP.OpenFile(mFileName.c_str(), true, true))
	{
		mCurrentFP.CloseFile();
		DisplayOut = L"Could not open/Create ";
		DisplayOut += mFileName;
		DisplayOut += L" output file!\n";
		return false;
	}
	mFileOpen = true;
	int fileversion = 0x104;
	mCurrentFP.Write("icsbin", 6, read);
	mCurrentFP.Write(&fileversion, 4, read);
	return true;
}

bool VSBIOWrite::WriteMessage(const std::vector<unsigned char>& message)
{
	return WriteMessage(&message[0], message.size());
}

bool VSBIOWrite::WriteMessage(const unsigned char *  message, const unsigned int& size)
{
	unsigned long read;
	mCurrentFP.Write(message, size, read);
	return true;
}
