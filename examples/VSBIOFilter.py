from ICS_VSBIO import VSBIOInterface as vsb
import sqlite3
import sys, os
import threading
from datetime import datetime, timezone

inFile = "./input.vsb"
outFile = "./output.db2"
filteredFile = "./filtered.vsb"
# Create a database containing all the messages
def createThreadFunc():
    vsb.CreateDatabase(inFile, outFile, None)

if os.path.isfile(outFile):
    print('Using existing database')
else:
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

# Open the message database
conn = sqlite3.connect(outFile, timeout=10)
conn.text_factory = lambda x: str(x, 'utf-8', 'ignore')

# Read the file start and end timestamp from the Networks table
cursor = conn.cursor()    
cursor.execute("SELECT Min(FirstTime), Max(LastTime) FROM Network")

row = cursor.fetchone()
if row is not None:
    startTime = row[0]
    sys.stdout.write('Start time: ' + datetime.fromtimestamp(startTime / 1e9, timezone.utc).isoformat() + '\n')
    endTime = row[1]
    sys.stdout.write('End time: ' + datetime.fromtimestamp(endTime / 1e9, timezone.utc).isoformat() + '\n')

filter = "(NetworkId > 0) " # Filter out the neoVI messages (NetworkId = 0)
if startTime and endTime: # limit between two timestamps (1/3 from start to 1/3 from end)
    delta = endTime - startTime
    filter += "AND (MessageTime > " + str(startTime + delta / 3) + ") AND (MessageTime < " + str(endTime - delta / 3) + ") "
filter += "AND (Id < 300)"  # For CAN data, limit to messages with certain Arb Ids

if vsb.WriteFilteredVsb(outFile, filteredFile, filter, None):
    sys.stdout.write('VSB file was created!\n')
else:
    sys.stdout.write('Error creating VSB file!\n')