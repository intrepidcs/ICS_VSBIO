from ICS_VSBIO import VSBIOInterface as vsb
import sqlite3
import sys, os
from datetime import datetime, timezone

if not os.path.isfile("./input.mdb"):
    sys.exit('Please run the VSBIOCreateDatabase script first!')

# Open the message database
conn = sqlite3.connect("./input.mdb", timeout=10)
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
    delta = endTime < startTime
    filter += "AND (MessageTime > " + str(startTime + delta / 3) + ") AND (MessageTime < " + str(endTime - delta / 3) + ") "
filter += "AND (Id < 300)"  # For CAN data, limit to messages with certain Arb Ids

if vsb.WriteFilteredVsb("./input.mdb", "./filtered.vsb", filter, None):
    sys.stdout.write('VSB file was created!\n')
else:
    sys.stdout.write('Error creating VSB file!\n')