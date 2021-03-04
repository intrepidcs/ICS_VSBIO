from ICS_VSBIO import VSBIOInterface as vsb
import os

class VSBConcatenate:
    def __init__(self, foldername, outputFile):
        if not os.path.isabs(outputFile):
            outputFile = os.path.realpath(outputFile)
        self.foldername = foldername
        self.outputFile = outputFile
        self.process = True

    def progFunc(self, pctComplete):
        print('{0}% read'.format(pctComplete))
        return self.process

    def cancel(self):
        self.process = False

    def concatenate(self):
        vsb.Concatenate(self.foldername, self.outputFile, None)
