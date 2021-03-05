from ICS_VSBIO import VSBReader  as reader
from ICS_VSBIO import VSBIOFlags as flags
from datetime import datetime, timezone

vsbRead = reader.VSBReader("input.vsb")

class NetworkInfo:
    def __init__(self, id):
        self.id = id
        self.firstTime = 0
        self.lastTime = 0
        self.numMessages = 0

    def processMessage(self, msg):
        timestamp = vsbRead.get_message_time(msg)
        if self.lastTime == 0 or self.firstTime > timestamp:
            self.firstTime = timestamp
        if self.lastTime < timestamp:
            self.lastTime = timestamp
        self.numMessages += 1

    def getTimeString(self, timeVal):
        return datetime.fromtimestamp(timeVal + 1167609600.0, timezone.utc).isoformat()

    def showInfo(self):
        if self.id == -1:
            print("File has {} messages, start time(UTC): {}, end time(UTC): {}".format(self.numMessages, self.getTimeString(self.firstTime), \
                self.getTimeString(self.lastTime)))
        else:
            print("Network {}, has {} messages, start time(UTC): {}, end time(UTC): {}".format(self.id, self.numMessages, \
                self.getTimeString(self.firstTime), self.getTimeString(self.lastTime)))
        

class VSBInfo:
    def __init__(self, filename):
        self.filename = filename
        self.networks = {}
        self.networks[-1] = NetworkInfo(-1)

    def processMessage(self, msg):
        if self.networks.get(msg.info.NetworkID) is None:
            self.networks[msg.info.NetworkID] = NetworkInfo(msg.info.NetworkID)
        self.networks[msg.info.NetworkID].processMessage(msg)
        self.networks[-1].processMessage(msg)  # keep the file start/end

    def progFunc(self, pctComplete):
        print('{0}% of file read'.format(pctComplete))

    def showInfo(self):
        for network in self.networks.values():
            network.showInfo()

vsbInfo = VSBInfo(vsbRead.filename)

count = 0
print('start')
try:
    for msg in vsbRead:
        count += 1
        if not count % 2000:
            print('{0}% of file read'.format(vsbRead.get_progress()))

        vsbInfo.processMessage(msg)
            
except ValueError as e:
    print(str(e))
except Exception as ex:
    print(ex)
else:
    vsbInfo.showInfo()


