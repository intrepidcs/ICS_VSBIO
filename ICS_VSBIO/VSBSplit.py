from ICS_VSBIO import VSBIOInterface as vsb
import os

class VSBSplit:
    def __init__(self, filename, numMessages):
        if not os.path.isabs(filename):
            filename = os.path.realpath(filename)
        self.filename = filename
        self.numMessages = numMessages
        self.process = True

    def progFunc(self, pctComplete):
        print('{0}% of file read'.format(pctComplete))
        return self.process

    def cancel(self):
        self.process = False

    def split(self):
        # split the file
        vsb.Split(self.filename, self.numMessages, os.path.dirname(self.filename), self.progFunc)
