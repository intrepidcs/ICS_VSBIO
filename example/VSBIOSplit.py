from ICS_VSBIO import VSBSplit as splitter
import threading

vsbSplit = splitter.VSBSplit("input.vsb", 100)

def threadFunc():
    vsbSplit.split()

print('start')
try:
    splitThread = threading.Thread(target=threadFunc)

    splitThread.start()    # Start splitting

	# If we want to cancel, use this call: vsbSplit.cancel()

    splitThread.join()   # Wait for it
			
except ValueError as e:
	print(str(e))
except:
	print('an unknown error has occurred')
else:
	print('Success')