from ICS_VSBIO import VSBConcatenate as concat
import threading

concat = concat.VSBConcatenate(".", "Combined.vsb")

def threadFunc():
    concat.split()

print('start')
try:
    concatThread = threading.Thread(target=threadFunc)

    concatThread.start()    # Start combining

	# If we want to cancel, use this call: concat.cancel()

    concatThread.join()   # Wait for it
			
except ValueError as e:
	print(str(e))
except:
	print('an unknown error has occurred')
else:
	print('Success')