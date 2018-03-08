from VSBIO import VSBIOInterface as vsb
import os

class VSBReader:
	def __init__(self, filename):
		self.size = 64
		self.message = vsb.VSBIOMalloc(self.size)
		if not os.path.isabs(filename):
			filename = os.path.realpath(filename)
		self.handle = vsb.ReadVSB(filename)
 
	def __del__(self):
		vsb.VSBIOFree(self.message)
		vsb.ReadClose(self.handle)

	def read_next_message(self):
		"""  get next message from the buffer.  """
		retunedval, size = vsb.ReadNextMessage(self.handle, self.message, self.size)
		if retunedval == vsb.eSuccess:
			return {"status":vsb.eSuccess, "Msg":self.message, "EDP":vsb.GetEDP(self.message), "sizeOfMsg":size}
		elif retunedval == vsb.eEndOfFile:
			vsb.ReadClose(self.handle)
			return {"status":vsb.eEndOfFile}
		elif retunedval == vsb.eError:
			return {"status": vsb.eError}
		elif retunedval == vsb.eBufferToSmall:
			vsb.VSBIOFree(self.message)
			self.message = vsb.VSBIOMalloc(size)
			self.size = size
			return self.read_next_message()