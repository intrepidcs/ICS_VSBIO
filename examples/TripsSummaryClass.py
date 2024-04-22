import os
import logging
from ICS_VSBIO import VSBIOInterface as vsb
from datetime import datetime, timezone
import sqlite3
import sys
import zipfile
import shutil
from pathlib import Path
from ICS_VSBIO import ICSFileInterfaceLibrary
#from MsgFileClass import msgFiles

class tripsSummary:
	def __init__(self, msgFilesList, ReportGenTimeStamp, log):
		self.TripMsgFiles = msgFilesList
		self.NumberOfMsgFiles = len(msgFilesList)
		self.ReportGenTimeStamp = ReportGenTimeStamp
		self.log = log
		self.NumberOfTrips = 1
		self.TripStartTime = []
		self.TripEndTime = []
		self.TripStartTimeRaw = []
		self.TripEndTimeRaw = []
		self.TripDurationMinutes = []
		self.NumberOfFilesInTrip = []
		self.FirstMsgFileNameInTrip = []
		self.LastMsgFileNameInTrip = []
		self.MaxTimeGapBetweenFilesInTrip = []
		self.MinTimeGapBetweenFilesInTrip = []		
		self.MaxTimeDeltaPerTripPerNetwork = []
		self.MinTimeDeltaPerTripPerNetwork = []
		self.AverageMessageRatePerTripPerNetwork = []
		self.GPSCoordinatesAtStartOfTrip = 0
		self.GPSCoordinatesAtEndOfTrip = 0
		self.TimeGapThreshToDeclareNewTrip = 3

		#first file in list is assumed to be a new trip
		self.TripStartTime.append(msgFilesList[0].FileStartTime)
		self.TripStartTimeRaw.append(msgFilesList[0].FileStartTimeRaw)
		self.FirstMsgFileNameInTrip.append(msgFilesList[0].InputFileName)
		self.LastMsgFileNameInTrip.append(msgFilesList[0].InputFileName)
		self.NumberOfFilesInTrip.append(1)
		self.MaxTimeGapBetweenFilesInTrip.append(-1000)
		self.MinTimeGapBetweenFilesInTrip.append(1000)

		for i in range(len(msgFilesList)-1):
			FileTimeDelta = (msgFilesList[i+1].FileStartTimeRaw / 1e9) - (msgFilesList[i].FileEndTimeRaw / 1e9)
			if (FileTimeDelta > self.TimeGapThreshToDeclareNewTrip):
				self.NumberOfTrips = self.NumberOfTrips + 1
				self.TripEndTime.append(msgFilesList[i].FileEndTime)
				self.TripEndTimeRaw.append(msgFilesList[i].FileEndTimeRaw)
				self.TripDurationMinutes.append( (self.TripEndTimeRaw[-1]/1e9 - self.TripStartTimeRaw[-1]/1e9) / 60)
				self.TripStartTime.append(msgFilesList[i+1].FileStartTime)
				self.TripStartTimeRaw.append(msgFilesList[i+1].FileStartTimeRaw)
				self.FirstMsgFileNameInTrip.append(msgFilesList[i+1].InputFileName)
				self.LastMsgFileNameInTrip.append(msgFilesList[i+1].InputFileName)
				self.NumberOfFilesInTrip.append(1)
				self.MaxTimeGapBetweenFilesInTrip.append(-1000)
				self.MinTimeGapBetweenFilesInTrip.append(1000)
			else:
				self.LastMsgFileNameInTrip.append(msgFilesList[i].InputFileName)
				if FileTimeDelta > self.MaxTimeGapBetweenFilesInTrip[-1]:
					self.MaxTimeGapBetweenFilesInTrip[-1] = FileTimeDelta
				if FileTimeDelta < self.MinTimeGapBetweenFilesInTrip[-1]:
					self.MinTimeGapBetweenFilesInTrip[-1] = FileTimeDelta
				self.NumberOfFilesInTrip[self.NumberOfTrips-1] = self.NumberOfFilesInTrip[self.NumberOfTrips-1] + 1

		self.TripEndTime.append(msgFilesList[len(msgFilesList)-1].FileEndTime)
		self.TripEndTimeRaw.append(msgFilesList[len(msgFilesList)-1].FileEndTimeRaw)
		self.TripDurationMinutes.append( (self.TripEndTimeRaw[-1]/1e9 - self.TripStartTimeRaw[-1]/1e9) / 60)





