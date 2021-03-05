from ICS_VSBIO import VSBSplit as splitter
from ICS_VSBIO import VSBConcatenate as concat

import threading
import os
import sys

if not os.path.isdir("./Out"):
    os.mkdir("./Out")

split = splitter.VSBSplit("input.vsb", 5000, "./Out")

def splitThreadFunc():
    split.split()

print('Starting to split!')
try:
    splitThread = threading.Thread(target=splitThreadFunc)

    splitThread.start()    # Start splitting

	# If we want to cancel, use this call: vsbSplit.cancel()

    splitThread.join()   # Wait for it
			
except ValueError as e:
	sys.exit(str(e))
except Exception as ex:
	print(ex)
else:
	print('Success splitting file!\n')

concat = concat.VSBConcatenate("./Out", "./Combined.vsb")

def concatThreadFunc():
    concat.concatenate()

print('Starting to concatenate!')
try:
    concatThread = threading.Thread(target=concatThreadFunc)

    concatThread.start()    # Start combining

	# If we want to cancel, use this call: concat.cancel()

    concatThread.join()   # Wait for it
			
except ValueError as e:
	print(str(e))
except Exception as ex:
    print(ex)
else:
	print('Success concatenating files!')