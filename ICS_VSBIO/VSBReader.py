from ICS_VSBIO import VSBIOInterface as vsb
import os

class ReadStatus:
	eSuccess = vsb.eSuccess
	eEndOfFile = vsb.eEndOfFile
	eError = vsb.eError
	eBufferTooSmall = vsb.eBufferTooSmall
	eInvalidPayloadSize = vsb.eInvalidPayloadSize
	eFileOpened = 10
	eInit = 11

class VSBMessage:
	def __init__(self, Msg, EDP, sizeOfMsg):
		self.info = Msg
		self.exData = EDP
		self.sizeOfMsg = sizeOfMsg

	def get_byte_from_data(self, index):
		if index > self.sizeOfMsg:
			raise ValueError("index greater then size")
		return vsb.GetByteFromData(self.info, index)

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
		if self.state != vsb.eEndOfFile :
			vsb.ReadClose(self.handle)

	def __iter__(self):
		if self.state == ReadStatus.eInit:
			raise ValueError('VSBReader has not been initialized')
		elif self.state == ReadStatus.eEndOfFile:
			self.__fileOpen(self.filename)
			if self.state != ReadStatus.eFileOpened:
				raise ValueError('VSB file has not been opened')
		elif self.state == ReadStatus.eError:
			vsb.ReadClose(self.handle)
			self.__fileOpen(self.filename)
			if self.state != ReadStatus.eFileOpened:
				raise ValueError('VSB file has not been opened')
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
			raise ValueError(self.get_error_message())
		elif self.state == vsb.eBufferTooSmall:
			vsb.VSBIOFree(self.message)
			self.message = vsb.VSBIOMalloc(size)
			self.size = size
			return self.__next__()

	def get_progress(self):
		'''
		returns the progress as a integer percentage
		'''
		return vsb.GetProgress(self.handle)

	def get_error_message(self):
		'''
		return Error messages if any.
		'''
		return vsb.GetErrorMessage(self.handle)

	def get_display_message(self):
		'''
		return Display messages if any
		'''
		return vsb.GetDisplayMessage(self.handle)

	def get_message_time(self, msg):
		'''
		return the message seconds since Jan 1, 2007
		'''
		return vsb.GetMsgTime(msg.info)

	def get_status(self):
		'''
		returns the current state
		'''
		return self.state

	def get_status_as_string(self):
		'''
		returns the current state in string format
		'''
		if self.state == ReadStatus.eSuccess:
			return 'Messages read successfully'
		elif self.state == ReadStatus.eError:
			return 'Error'
		elif self.state == ReadStatus.eEndOfFile:
			return 'End Of File'
		elif self.state == ReadStatus.eFileOpened:
			return 'File Opened'
		elif self.state == ReadStatus.eInit:
			return 'Init'
