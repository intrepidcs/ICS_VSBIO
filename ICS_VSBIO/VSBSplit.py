from ICS_VSBIO import VSBIOInterface as vsb

class VSBSplit:
    def __init__(self, filename, numMessages, outDir):
        if not os.path.isabs(filename):
            filename = os.path.realpath(filename)
        self.filename = filename
        self.numMessages = numMessages
        self.outDir = outDir
        self.process = True

    def progFunc(self, pctComplete):
        print('{0}% of file read'.format(pctComplete))
        return self.process

    def cancel(self):
        self.process = False

    def split(self):
        # split the file
        vsb.Split(self.filename, self.numMessages, self.outDir, self.progFunc)
