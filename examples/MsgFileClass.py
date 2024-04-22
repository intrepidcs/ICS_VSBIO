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

class msgFiles:
	def __init__(self, inputFilePaths, ReportGenTimeStamp, OutputFilePath, log):
		self.FilesList = []
		self.FilesListSorted = []
		self.OutputFilePath = OutputFilePath
		self.log = log

		for inputFilePath in inputFilePaths:
			self.FilesList.append(msgFile())
			self.FilesList[-1].InputFileName = os.path.basename(inputFilePath["path"])
			self.FilesList[-1].InputFilePath = os.path.dirname(inputFilePath["path"])
			filename, fileExtension = os.path.splitext(str(self.FilesList[-1].InputFileName))
			self.FilesList[-1].DB_FileName = os.path.join(self.OutputFilePath, filename + "_" + ReportGenTimeStamp + "_Filtrd.db2") 
			self.FilesList[-1].OutputFilePath = self.OutputFilePath
			self.FilesList[-1].FileExtension = fileExtension
			self.FilesList[-1].FileCreatedByClass = False
			self.FilesList[-1].FileStartTime = ""
			self.FilesList[-1].FileEndTime = ""
			self.FilesList[-1].FileStartTimeRaw = 0
			self.FilesList[-1].FileEndTimeRaw = 0
			self.FilesList[-1].TempVSBExtracted = False
			zip2FolderName = ""

			self.log.info("InputFileName: " + str(self.FilesList[-1].InputFileName))
			#now compress output files
			if (fileExtension == ".zip"):
				with zipfile.ZipFile(os.path.join(self.FilesList[-1].InputFilePath, self.FilesList[-1].InputFileName), 'r') as zipObj:
					# Get a list of all archived file names from the zip
					listOfFileNames = zipObj.namelist()
					# Iterate over the file names
					self.log.info("First level zip filenames:")

					for zipfileName in listOfFileNames:
						self.log.info(zipfileName)

						# skip directories
						if not zipfileName:
							continue

						if zipfileName[len(zipfileName)-4:len(zipfileName)] == ('.vsb'):
							zipObj.extract(zipfileName, self.FilesList[-1].OutputFilePath)
							self.FilesList[-1].InputFileName = zipfileName
							self.FilesList[-1].FileExtension = ".vsb"
							self.FilesList[-1].InputFilePath = self.FilesList[-1].OutputFilePath
							self.FilesList[-1].TempVSBExtracted = True
							self.log.info("Only one level of zip")

						elif zipfileName[len(zipfileName)-8:len(zipfileName)] == ('.vsb.zip'):
							self.log.info(".vsb.zip file found")
							zip2FolderName = os.path.join(OutputFilePath.replace('\\', '/'),'tempZipFolderName').replace('\\', '/')
							zipObj.extract(zipfileName, zip2FolderName)
							zip3FileName = os.path.join(zip2FolderName,zipfileName).replace('\\', '/')
							with zipfile.ZipFile(zip3FileName) as zipObj2:
								zipObj2.extract(zipObj2.namelist()[0], OutputFilePath)
								self.FilesList[-1].InputFileName = zipObj2.namelist()[0]
								self.log.info(str(self.FilesList[-1].InputFileName))
								filename2, fileExtension2 = os.path.splitext(self.FilesList[-1].InputFileName)
								self.FilesList[-1].FileExtension = fileExtension2
								self.FilesList[-1].InputFilePath = self.FilesList[-1].OutputFilePath
								self.FilesList[-1].TempVSBExtracted = True
							try:
								shutil.rmtree(zip2FolderName)
								self.log.info("Removed top level zip file")
								
							except OSError as e:
								a=a

			if (self.FilesList[-1].FileExtension == ".vsb"):
				try:
					# Open the message database
					self.log.info("Create database for file " + str(os.path.join(self.FilesList[-1].InputFilePath, self.FilesList[-1].InputFileName)))
					self.log.info("vsb filename: " + str(os.path.join(self.FilesList[-1].InputFilePath, self.FilesList[-1].InputFileName)))
					vsb.CreateDatabase(os.path.join(self.FilesList[-1].InputFilePath, self.FilesList[-1].InputFileName), self.FilesList[-1].DB_FileName, None)
					if ((self.FilesList[-1].TempVSBExtracted == True) and (os.path.isfile(os.path.join(self.FilesList[-1].InputFilePath, self.FilesList[-1].InputFileName)))):
						os.remove(os.path.join(self.FilesList[-1].InputFilePath, self.FilesList[-1].InputFileName))
						self.log.info("Removed temp input vsb file")
					self.FilesList[-1].FileCreatedByClass = True
					conn = sqlite3.connect(self.FilesList[-1].DB_FileName, timeout=10)

				except ValueError as e:
					print(str(e))
			else:
				self.FilesList[-1].DB_FileName = os.path.join(self.FilesList[-1].InputFilePath, self.FilesList[-1].InputFileName)
				conn = sqlite3.connect(os.path.join(self.FilesList[-1].InputFilePath, self.FilesList[-1].InputFileName), timeout=10)

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
				self.FilesList[-1].FileStartTime = self.FilesList[-1].FileStartTime[0:len(self.FilesList[-1].FileStartTime)-7]
				endTime = row[1]
				self.FilesList[-1].FileEndTimeRaw = endTime
				self.FilesList[-1].FileEndTime = (datetime.fromtimestamp(endTime / 1e9, timezone.utc).isoformat() + '\n')
				self.FilesList[-1].FileEndTime = self.FilesList[-1].FileEndTime[0:len(self.FilesList[-1].FileEndTime)-7]				
				tempTime = (self.FilesList[-1].FileEndTimeRaw - self.FilesList[-1].FileStartTimeRaw)/1e9
				self.FilesList[-1].FileDurationInMin = tempTime / 60

			#now get the list of existing network Ids and Names
			cursor.execute("Select DISTINCT Id, Name from Network ORDER by Id")

			row = cursor.fetchall()
			if row is not None:
				for i in range(len(row)):
					self.FilesList[-1].FileNetworks.append(file_network())
					self.FilesList[-1].FileNetworks[-1].network_id = row[i][0]
					self.FilesList[-1].FileNetworks[-1].network_name = row[i][1]

			# now get the list of MsgIDs for each network
			# NumMessages is an alias, which can be used as a shortcut in the where

			for i in range(len(self.FilesList[-1].FileNetworks)):
				self.FilesList[-1].FileNetworks[i].numberOfArbIDs = 0
				self.FilesList[-1].FileNetworks[i].network_tot_number_messages_in_file = 0
				cursor.execute("SELECT Id, COUNT(*) NumMessages FROM RawMessageData WHERE NetworkId = " + str(self.FilesList[-1].FileNetworks[i].network_id) + " GROUP BY Id")
				row = cursor.fetchall()
				for j in range(len(row)):
					self.FilesList[-1].FileNetworks[i].network_msg_ids_dec.append(row[j][0])
					self.FilesList[-1].FileNetworks[i].network_msg_ids_hex.append(hex(row[j][0]))
					self.FilesList[-1].FileNetworks[i].network_msg_num_msgs.append(row[j][1])
					self.FilesList[-1].FileNetworks[i].numberOfArbIDs = self.FilesList[-1].FileNetworks[i].numberOfArbIDs + 1
					self.FilesList[-1].FileNetworks[i].network_tot_number_messages_in_file = self.FilesList[-1].FileNetworks[i].network_tot_number_messages_in_file + self.FilesList[-1].FileNetworks[i].network_msg_num_msgs[-1]

			# now loop through each network and each ArbID and determine the min and max sample period
			for i in range(len(self.FilesList[-1].FileNetworks)):
				for j in range(self.FilesList[-1].FileNetworks[i].numberOfArbIDs):
					if self.FilesList[-1].FileNetworks[i].network_msg_num_msgs[j] > 1:
						ViewString ="TempView" + str(i) + "_" + str(j)
						QueryString = "CREATE VIEW IF NOT EXISTS " + ViewString + " AS SELECT * FROM RawMessageData WHERE NetworkId = " + str(self.FilesList[-1].FileNetworks[i].network_id) + " AND ID = " + str(self.FilesList[-1].FileNetworks[i].network_msg_ids_dec[j])
						cursor.execute(QueryString)
						QueryString = "SELECT MIN(DeltaVal), MAX(DeltaVal) FROM (SELECT (a.MessageTime - b.MessageTime) as DeltaVal FROM " + ViewString + " AS a JOIN " + ViewString + " AS b ON (b.MessageTime = (SELECT MAX(z.MessageTime) FROM " + ViewString + " AS z WHERE z.MessageTime < a.MessageTime)))"
						cursor.execute(QueryString)
						row = cursor.fetchall()
						self.FilesList[-1].FileNetworks[i].network_msg_id_min_periods.append(row[0][0]*1e-9)
						self.FilesList[-1].FileNetworks[i].network_msg_id_max_periods.append(row[0][1]*1e-9)
						cursor.execute("DROP VIEW IF EXISTS " + ViewString)					
					else:
						self.FilesList[-1].FileNetworks[i].network_msg_id_min_periods.append(0)
						self.FilesList[-1].FileNetworks[i].network_msg_id_max_periods.append(0)

				#now calculate the overall min and max time gaps for all messages on network
				if self.FilesList[-1].FileNetworks[i].network_tot_number_messages_in_file > 1:
					QueryString = "CREATE VIEW IF NOT EXISTS TempView5 AS SELECT * FROM RawMessageData WHERE NetworkId = " + str(self.FilesList[-1].FileNetworks[i].network_id)
					cursor.execute(QueryString)
					QueryString = "SELECT MIN(DeltaVal), MAX(DeltaVal) FROM (SELECT (a.MessageTime - b.MessageTime) as DeltaVal FROM TempView5 AS a JOIN TempView5 AS b ON (b.MessageTime = (SELECT MAX(z.MessageTime) FROM TempView5 AS z WHERE z.MessageTime < a.MessageTime)))"
					cursor.execute(QueryString)
					row = cursor.fetchall()
					self.FilesList[-1].FileNetworks[i].network_min_time_gap = (row[0][0]*1e-9)
					self.FilesList[-1].FileNetworks[i].network_max_time_gap = (row[0][1]*1e-9)

					QueryString = "SELECT MIN(MessageTime), MAX(MessageTime) FROM TempView5"
					cursor.execute(QueryString)
					row = cursor.fetchall()
					tempTime = row[0][0]
					self.FilesList[-1].FileNetworks[i].network_data_start_time = (datetime.fromtimestamp(tempTime / 1e9, timezone.utc).isoformat() + '\n')
					self.FilesList[-1].FileNetworks[i].network_data_start_time = self.FilesList[-1].FileNetworks[i].network_data_start_time[0:len(self.FilesList[-1].FileNetworks[i].network_data_start_time)-7]
					tempTime = row[0][1]
					self.FilesList[-1].FileNetworks[i].network_data_end_time = (datetime.fromtimestamp(tempTime / 1e9, timezone.utc).isoformat() + '\n')
					self.FilesList[-1].FileNetworks[i].network_data_end_time = self.FilesList[-1].FileNetworks[i].network_data_end_time[0:len(self.FilesList[-1].FileNetworks[i].network_data_end_time)-7]
					cursor.execute("DROP VIEW IF EXISTS TempView5")
								
				else:
					self.FilesList[-1].FileNetworks[i].network_min_time_gap = 0
					self.FilesList[-1].FileNetworks[i].network_max_time_gap = 0
					self.FilesList[-1].FileNetworks[i].network_data_start_time = 0
					self.FilesList[-1].FileNetworks[i].network_data_end_time = 0

		self.FilesListSorted = sorted(self.FilesList, key=lambda x: x.FileStartTimeRaw)

		#now calculate and populate the time gap between each file now that they are sorted
		self.FilesListSorted[0].time_gap_from_prev_file = 0
		for i in range(1, len(self.FilesListSorted)):
			self.FilesListSorted[i].time_gap_from_prev_file = (self.FilesListSorted[i].FileStartTimeRaw - self.FilesListSorted[i-1].FileEndTimeRaw) /1e9

class msgFile:
	def __init__(self):
		self.DB_FileName = ""
		self.FilteredVSBFilename = ""
		self.NumberOfRecordsInFilteredVSB = 0		
		self.FileStartTimeRaw = 0.0
		self.FileStartTime = 0.0
		self.FileEndTimeRaw = 0.0
		self.FileEndTime = 0.0
		self.InputFileName = ""
		self.InputFilePath = ""
		self.OutputFilePath = ""
		self.FilePath = ""
		self.FileExtension = ""
		self.FileNetworks = []
		self.FileCreatedByClass = False
		self.time_gap_from_prev_file = 0
		self.FileDurationInMin = 0

class file_network:
	def __init__(self):
		self.network_name = ""
		self.network_id = 0
		self.network_msg_ids_dec = []
		self.network_msg_ids_hex = []
		self.network_msg_num_msgs = []
		self.network_msg_id_min_periods = []
		self.network_msg_id_max_periods = []
		self.network_tot_number_messages_in_file = 0
		self.network_data_start_time = ""
		self.network_data_end_time = ""
		self.network_min_time_gap = 0.0
		self.network_max_time_gap = 0.0