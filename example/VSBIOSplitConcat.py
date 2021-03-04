from ICS_VSBIO import VSBSplit as splitter
from ICS_VSBIO import VSBConcatenate as concat

import threading
import os
import sys

os.mkdir("./Out")
split = splitter.VSBSplit("input.vsb", 5000, "./Out")

def splitThreadFunc():
    split.split()

print('start')
try:
    splitThread = threading.Thread(target=splitThreadFunc)

    splitThread.start()    # Start splitting

	# If we want to cancel, use this call: vsbSplit.cancel()

    splitThread.join()   # Wait for it
			
except ValueError as e:
	sys.exit(str(e))
except:
	sys.exit('an unknown error has occurred while splitting')
else:
	print('Success splitting file!\n')

concat = concat.VSBConcatenate("./Out", "./Combined.vsb")

def concatThreadFunc():
    concat.split()

print('start')
try:
    concatThread = threading.Thread(target=concatThreadFunc)

    concatThread.start()    # Start combining

	# If we want to cancel, use this call: concat.cancel()

    concatThread.join()   # Wait for it
			
except ValueError as e:
	print(str(e))
except:
	print('an unknown error has occurred while concatenating')
else:
	print('Success concatenating files!')