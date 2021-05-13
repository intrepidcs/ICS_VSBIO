import os
import logging
from ICS_VSBIO import VSBIOInterface as vsb
from datetime import datetime, timezone
import sqlite3
import sys
from ICS_IPA import IPAInterfaceLibrary

class msgFiles:
	def __init__(self, inputFilePaths, ReportGenTimeStamp):
		self.FilesList = []
		self.FilesListSorted = []

		logging.basicConfig(level=logging.INFO)
		self.log = logging.getLogger(__name__)
		self.handler = logging.FileHandler('IPA.log')
		self.handler.setLevel(logging.INFO)

		# create a logging format
		self.formatter = logging.Formatter('%(asctime)s - %(name)s - %(message)s')
		self.handler.setFormatter(self.formatter)
		self.log.addHandler(self.handler)

		if IPAInterfaceLibrary.is_running_on_wivi_server():
			OutputFilePath = os.path.dirname(sys.argv[0]) + "\\"
		else:
			OutputFilePath = os.path.dirname(sys.argv[0]) + "\\"

		for inputFilePath in inputFilePaths:
			filenameWithoutPath = os.path.basename(inputFilePath["path"])
			filename, fileExtension = os.path.splitext(filenameWithoutPath)
			self.FilesList.append(msgFile())
			self.FilesList[-1].DB_FileName = OutputFilePath + filename + "_" + ReportGenTimeStamp + "_Filtrd.db2"
			self.FilesList[-1].FileName = filenameWithoutPath
			self.FilesList[-1].FilePath = OutputFilePath
			self.FilesList[-1].FileExtension = fileExtension
			self.FilesList[-1].FileCreatedByClass = False
			self.FilesList[-1].FileStartTime = ""
			self.FilesList[-1].FileEndTime = ""
			self.FilesList[-1].FileStartTimeRaw = 0
			self.FilesList[-1].FileEndTimeRaw = 0

			if (fileExtension == ".vsb"):
				try:
					# Open the message database
					self.log.info("Create database for file " + inputFilePath["path"])
					vsb.CreateDatabase(inputFilePath["path"], self.FilesList[-1].DB_FileName, None)
					self.FilesList[-1].FileCreatedByClass = True
					conn = sqlite3.connect(self.FilesList[-1].DB_FileName, timeout=10)

				except ValueError as e:
					print(str(e))
			else:
				self.FilesList[-1].DB_FileName = inputFilePath["path"]
				conn = sqlite3.connect(inputFilePath["path"], timeout=10)

			self.log.info("Get info from DB file")
			conn.text_factory = lambda x: str(x, 'utf-8', 'ignore')

			# first get the list of networks and their IDs
			cursor = conn.cursor()  
			cursor.execute("SELECT Min(FirstTime), Max(LastTime) FROM Network")

			row = cursor.fetchone()
			if row is not None:
				startTime = row[0]
				self.FilesList[-1].FileStartTimeRaw = startTime
				self.FilesList[-1].FileStartTime = (datetime.fromtimestamp(startTime / 1e9, timezone.utc).isoformat() + '\n')
				endTime = row[1]
				self.FilesList[-1].FileEndTimeRaw = endTime
				self.FilesList[-1].FileEndTime = (datetime.fromtimestamp(endTime / 1e9, timezone.utc).isoformat() + '\n')

			#now get the list of existing network Ids and Names
			cursor.execute("Select DISTINCT Id, Name from Network ORDER by Id")

			row = cursor.fetchall()
			if row is not None:
				for i in range(len(row)):
					self.FilesList[-1].FileneNetworks.append(file_network())
					self.FilesList[-1].FileneNetworks[-1].network_id = row[i][0]
					self.FilesList[-1].FileneNetworks[-1].network_name = row[i][1]

			# now get the list of MsgIDs for each network
			# NumMessages is an alias, which can be used as a shortcut in the where

			for i in range(len(self.FilesList[-1].FileneNetworks)):
				cursor.execute("SELECT Id, COUNT(*) NumMessages FROM RawMessageData WHERE NetworkId = " + str(self.FilesList[-1].FileneNetworks[i].network_id) + " GROUP BY Id")
				row = cursor.fetchall()
				for j in range(len(row)):
					self.FilesList[-1].FileneNetworks[i].network_msg_ids_dec.append(row[j][0])
					self.FilesList[-1].FileneNetworks[i].network_msg_ids_hex.append(hex(row[j][0]))
					self.FilesList[-1].FileneNetworks[i].network_msg_num_msgs.append(row[j][1])

		self.FilesListSorted = sorted(self.FilesList, key=lambda x: x.FileStartTimeRaw)				


class msgFile:
	def __init__(self):
		self.DB_FileName = ""
		self.FilteredVSBFilename = ""
		self.NumberOfRecordsInFilteredVSB = 0		
		self.FileStartTime = 0.0
		self.FileEndTime = 0.0
		self.FileName = ""
		self.FilePath = ""
		self.FileExtension = ""
		self.FileneNetworks = []
		self.FileCreatedByClass = False

class file_network:
	def __init__(self):
		self.network_name = ""
		self.network_id = 0
		self.network_msg_ids_dec = []
		self.network_msg_ids_hex = []
		self.network_msg_num_msgs = []
