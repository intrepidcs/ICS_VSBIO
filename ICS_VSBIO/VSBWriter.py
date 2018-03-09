from ICS_VSBIO  import VSBIOInterface as vsb
import os 

class VSBWriter:
    def __init__(self, filename):
        if not os.path.isabs(filename):
            filename = os.path.realpath(filename)
        self.handle = vsb.WriteVSB(filename)

    def __del__(self):
        vsb.WriteClose(self.handle)

    def write_msg(self, vsbMessage):
        ''' writes vsb message to file.'''
        vsb.WriteMessage(self.handle, vsbMessage.info, vsbMessage.sizeOfMsg)
