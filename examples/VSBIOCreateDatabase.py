from ICS_VSBIO import VSBIOInterface as vsb
import threading
import sys

# Create a database containing all the messages
def createThreadFunc():
    vsb.CreateDatabase("./input.vsb", "./input.mdb", None)

print('Starting to create the database!')
try:
    createThread = threading.Thread(target=createThreadFunc)

    createThread.start()    # Start the thread

    createThread.join()   # Wait for it
			
except ValueError as e:
	sys.exit(str(e))
except Exception as ex:
	print(ex)
else:
	print('Success creating the database!\n')