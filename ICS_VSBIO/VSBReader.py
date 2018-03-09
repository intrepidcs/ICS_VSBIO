from ICS_VSBIO import VSBIOInterface as vsb

import os

class ReadStatus:
	eSuccess = vsb.eSuccess
	eEndOfFile = vsb.eEndOfFile
	eError = vsb.eError
	eBufferToSmall = vsb.eBufferToSmall
	eFileOpened = 10
	eInit = 11

class VSBMessage:
	def __init__(self, Msg, EDP, sizeOfMsg):
		self.Msg = Msg
		self.EDP = EDP
		self.sizeOfMsg = sizeOfMsg

class VSBReader:
	def __init__(self, filename):
		self.size = 64
		self.state = ReadStatus.eInit
		self.__fileOpen(filename)

	def __fileOpen(self, filename):
		self.filename = filename
		self.message = vsb.VSBIOMalloc(self.size)
		if not os.path.isabs(filename):
			filename = os.path.realpath(filename)
		self.handle = vsb.ReadVSB(filename)
		self.state = ReadStatus.eFileOpened
 
	def __del__(self):
		vsb.VSBIOFree(self.message)
		vsb.ReadClose(self.handle)

	def __iter__(self):
		if self.state == ReadStatus.eInit:
			raise 
		elif self.state == ReadStatus.eEndOfFile:
			self.__fileOpen(self.filename)
			if self.state != ReadStatus.eFileOpened:
				raise
		elif self.state == ReadStatus.eError:
			vsb.ReadClose(self.handle)
			self.__fileOpen(self.filename)
			if self.state != ReadStatus.eFileOpened:
				raise
		return self

	def __next__(self):
		"""  get next message from the buffer.  """
		self.state, size = vsb.ReadNextMessage(self.handle, self.message, self.size)
		if self.state == vsb.eSuccess:
			return VSBMessage(self.message, vsb.GetEDP(self.message), size)
		elif self.state == vsb.eEndOfFile:
			vsb.ReadClose(self.handle)
			raise StopIteration()
		elif self.state == vsb.eError:
			raise ValueError(self.getErrorMessage())
		elif self.state == vsb.eBufferToSmall:
			vsb.VSBIOFree(self.message)
			self.message = vsb.VSBIOMalloc(size)
			self.size = size
			return self.read_next_message()

	def getProgress(self):
		return vsb.GetProgress(self.handle)

	def getErrorMessage(self):
		return vsb.GetErrorMessage(self.handle)

	def getDisplayMessage(self):
		return vsb.GetDisplayMessage(self.handle)

	def getStatus(self):
		return self.state

	def getStatusAsString(self):
		if self.state == ReadStatus.eSuccess:
			return 'Success'
		elif self.state == ReadStatus.eError:
			return 'Error'
		elif self.state == ReadStatus.eEndOfFile:
			return 'End Of File'
		elif self.state == ReadStatus.eFileOpened:
			return 'File Opened'
		elif self.state == ReadStature.eInit:
			return 'Init'
