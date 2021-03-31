// ---------------------------------------------------------------------------


#include "VSBIO.h"
#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif

#ifdef WIN32 
#define snprintf _snprintf
#endif

#include <stdio.h>
#include <string.h>
#include <limits>
#include <algorithm>

#define VSB_HEADERSIZE 10
#define VSB_2_3_EDPHEADERSIZE 16

// ---------------------------------------------------------------------------

VSBIORead::VSBIORead(const std::string& sFileNames) : mFullFileName(sFileNames)
{
	mCurrentMsgLocation = 0;
	mCurrentFileSize = 50000;// number over 64 so the first message does not closefile
	mDisplayOut = "";

    std::string sDirectory, sName, sExtension;
    SplitPath(mFullFileName, sDirectory, sName, sExtension);
	mFileName = sName + sExtension;

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
	return (int)((float)(mCurrentMsgLocation - mMsgStartLocation) / (float)(mCurrentFileSize - mMsgStartLocation) * 100);
}


VSBIORead::enumFileCondition VSBIORead::ReadNextMessage()
{
	return ReadNextMessage(vecMessage);
} 

VSBIORead::enumFileCondition VSBIORead::ReadNextMessage(std::vector<unsigned char>& msg)
{
	enumFileCondition returnvalue;
	if (msg.size() <  sizeof(VSBSpyMessage))
		msg.resize(sizeof(VSBSpyMessage));
	size_t retval;
	while ((returnvalue = ReadNextMessage(&msg[0], msg.size(), &retval)) == eBufferToSmall)	
		msg.resize(retval);	
	msg.resize(retval); //should always make the vector smaller
	return returnvalue;
}

VSBIORead::enumFileCondition VSBIORead::ReadNextMessage(unsigned char * msg, size_t msgBufferSize, size_t * returnLength )
{
	this->message = msg;
	this->messageBufferSize = msgBufferSize;
	mBufferSizeRequired = sizeof(VSBSpyMessage);
    *returnLength = (size_t)mBufferSizeRequired;
	if (mBufferSizeRequired < sizeof(VSBSpyMessage))
		return eBufferToSmall;

	unsigned long read;
	mDisplayOut = "";
	if (mCurrentMsgLocation + sizeof(VSBSpyMsgTime) >= mCurrentFileSize)
	{
		mDisplayOut += mFileName + ": Closed\n";
		mCurrentFile.CloseFile();//we ignore the time stamp at the end of 0x103 since 0x104 does not have it
		return eEndOfFile;
	}

	if (mCurrentFileType == VSBIONone)
	{
		if (!mCurrentFile.OpenFile(mFullFileName.c_str(), false, false))
		{
			mErrorOut += mFileName + ": Could Not Open\n";
			mCurrentFile.CloseFile();
			return eError;
		}

		char id[7];
		mCurrentFile.Read(id, 6, read);
		if ((read != 6) || (memcmp(id, "icsbin", 6) != 0))
		{
			mErrorOut += mFileName + ": Invalid vsb file\n";
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
			mErrorOut += mFileName + ": Invalid version\n";
			mCurrentFile.CloseFile();
			return eError;
		}
		mDisplayOut += mFileName + ": Opened\n";
		mCurrentFileSize = mCurrentFile.FileSizeLarge();
	}

	enumFileCondition condition = eError;
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
		default:
			mErrorOut += "Invalid File Type \n";
			return eError;
	}
	*returnLength = (size_t)mBufferSizeRequired;
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
	const unsigned int EDP_SECTION_LEN = 11; // strlen(EDP_SECTION);

	mCurrentEDPIndex = 1;
	unsigned long read;
	unsigned int EDPSize;
	unsigned long locationBefore = (unsigned long)mCurrentFile.GetFilePtrLong();
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

VSBIOReadMultiple::VSBIOReadMultiple(const std::vector<std::string> &files)
{
	for (std::vector<std::string>::const_iterator it = files.begin(); it != files.end(); ++it)
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
				mDisplayOut += "Terminated incorectly, an Error has occurred.\n";
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
	int percent = 0;
	for (ReadList::iterator it = IOReadList.begin(); it != IOReadList.end(); ++it) {
		percent += (*it)->GetProgress();
	}
	return percent / (int)IOReadList.size();
}

VSBIORead::enumFileCondition VSBIOReadMultiple::ReadNextMessage(std::vector<unsigned char> &msg)
{
	VSBIORead::enumFileCondition returnvalue;
	if (msg.size() <  sizeof(VSBSpyMessage))
		msg.resize(sizeof(VSBSpyMessage));
	size_t retval;
	while ((returnvalue = ReadNextMessage(&msg[0], msg.size(), &retval)) == VSBIORead::eBufferToSmall)
		msg.resize(retval);
	msg.resize(retval); //should always make the vector smaller
	return returnvalue;
}

VSBIORead::enumFileCondition VSBIOReadMultiple::ReadNextMessage(unsigned char * msg, size_t sizeOfBuffer, size_t * returnLength)
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

		memcpy(msg, pair.second->GetMessage(), pair.second->GetMessageSize());
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
			mDisplayOut += "Terminated incorectly, an Error has occurred.";
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

bool VSBIOWrite::Init(const std::string& fileName)
{
	unsigned long read;
	mFileName = fileName;
	if (mFileOpen)
		mCurrentFP.CloseFile();
	if (!mCurrentFP.OpenFile(mFileName.c_str(), true, true))
	{
		mCurrentFP.CloseFile();
		DisplayOut = "Could not open/Create ";
		DisplayOut += mFileName;
		DisplayOut += " output file!\n";
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
	return WriteMessage(&message[0], (unsigned int)message.size());
}

bool VSBIOWrite::WriteMessage(const unsigned char *  message, const unsigned int& size)
{
	unsigned long read;
	mCurrentFP.Write(message, size, read);
	return true;
}

bool VSBIOWrite::Concatenate(std::vector<std::string> &sInputFileList, ProgressFunc prog)
{
	VSBIOReadMultiple read(sInputFileList);

	unsigned long long counter = 1;
	std::vector<unsigned char> message;

	while (read.ReadNextMessage(message) == VSBIORead::eSuccess)
	{
		if (prog && !(counter++ % 100000))
        {
		    if (!prog(read.GetProgress()))
                break;
        }

		WriteMessage(message);
	}
    return true;
}

/// <summary>
/// Concatenates the vsb files found in the provided directory, sorted by name
/// </summary>
/// <param name="sInputFilePath">Directory where the input files reside</param>
/// <param name="prog">Progress return</param>
/// <returns>Whether the files were concatenated successfully</returns>
bool VSBIOWrite::ConcatenateFromDirectory(const std::string &sInputFilePath, ProgressFunc prog)
{
    std::vector<std::string> sInputFileList = GetFilesInDirectory(sInputFilePath, "*.vsb");
    std::stable_sort(sInputFileList.begin(), sInputFileList.end());
    return Concatenate(sInputFileList, prog);
}

/// <summary>
/// Splits this file using the messages per file parameter and a _00000... suffix for the output files.
/// </summary>
/// <param name="nMessagesPerFile">Number of messages to split on</param>
/// <param name="sOutputLocation">Output path</param>
/// <param name="prog">Progress callback</param>
/// <returns>Whether the output files were all written</returns>
bool VSBIORead::Split(const uint64_t& nMessagesPerFile, const std::string& sOutputLocation, ProgressFunc prog)
{
	VSBIOWrite write;

    std::string sDirectory, sName, sExtension;
    SplitPath(mFileName, sDirectory, sName, sExtension);

	unsigned long long counter = 0;
	unsigned int currentFileNumber = 0;
	std::string outputFileName;
	std::vector<unsigned char> msg;
	char szBuffer[81];
	while (ReadNextMessage(msg) == VSBIORead::eSuccess)
	{
		if (prog && !((counter + 1) % 100000))
        {
		    if (!prog(GetProgress()))
                break;
        }

		if (!(counter++ % nMessagesPerFile))
		{
			outputFileName = CombinePath(sOutputLocation, sName);
			snprintf(szBuffer, 81, "_%05d", (int)(currentFileNumber++));
			outputFileName += szBuffer;
			outputFileName += sExtension;
			write.Init(outputFileName);
		}
		write.WriteMessage(msg);
	}
    return true;
}

