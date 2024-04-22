##########################################################################################################################################################################################################################################
#    Script Description:
#        This script opens a list of network message data files (vsb or db2) and applies a filter to each file extracting the records that match the filter criteria.  
#        It will make a corresponding vsb file for each input vsb or db2 file. Depending on the settings in the config file it will also combine the resulting files 
#        into a single vsb file. If the config specifies to combine the vsb files it will delete the individual vsb files after combining them. 
#
###########################################################################################################################################################################################################################################
#    Script Inputs: When you run this script you will be prompted with 2 file open dialog windows. The first asks for a config file with extension *.asl; the second asks
#    for a list of one or more network data files (*.vsb, *.db2)
#
#         Script config file SplitVSB_ByArbIDAndNetwork_Config.asl is a JSON file used to configure the script. This file has the following keys:
#
#             StartTimeInSecondsFromStartOfFile - Time in seconds from the start of the file in which you want to begin extracting data. If using absolut time values set this to NaN
#
#             EndTimeInSecondsFromStartOfFile - Time in seconds from the start of the file in which you want to end extracting data. If using absolut time values set this to NaN
#
#             StartTimeAbsolute - Absolute time (including date) at which you want to begin extraction. Example format in UTC time 2021-03-15T08:55:50.070000+00:00
#
#             EndTimeAbsolute - Absolute time (including date) at which you want to end extraction. Example format in UTC time 2021-03-15T08:55:50.070000+00:00
#
#             DeleteTempDB2FileAfterExecution - Flag can be TRUE or FALSE. If TRUE it will delete any temporary db2 if they were made by the script. Any db2 files that were
#                                               input to the script will not be deleted
#
#             CombineResultingFilesToASingleVSB - Flag can be TRUE or FALSE. If TRUE, the resulting list of VSB files will be combined into a single file. 
#
#			  MsgIDsPerNetwork - List of MsgIDs for each network you would like to extract to output files. If left blank then all network data will be returned. 
#
#					Note that you can use wild characters in the MsgID list ie:
#					"MsgIDsPerNetwork": [{
#						"network_IDString": "HSCAN",
#						"MsgIdList": "0x7*, 0x6??"
#
#        Network data file(s) list. Files can be in vsb or db2 format or mixture of these.
#
##########################################################################################################################################################################################################################################
#    Script Outputs:
#        The script outputs a two files (vsb + db2) that both start with the text CombinedNetworkFile_ followed by a timestamp indicating when the script was run. 
#
##########################################################################################################################################################################################################################################

import numpy as np
import pprint
import enum
import sys
import os
import logging
import json
import re
import sqlite3
import sys, os
import math
import zipfile

from UtilityFunctions import ConvertNetworkStringToID
from ICS_VSBIO import ICSFileInterfaceLibrary
from shutil import copyfile

from datetime import date, datetime, time, timezone

from ICS_VSBIO import VSBIOInterface as vsb
from time import mktime
from MsgFileClass import msgFiles

slFilePath = ICSFileInterfaceLibrary.get_config_file()
inputFilePaths = ICSFileInterfaceLibrary.get_input_file_list()

ReportGenTimeStamp = datetime.now().strftime("%m-%d-%y_%H-%M-%S")

logging.basicConfig(level=logging.INFO)
log = logging.getLogger(__name__)
loggingPath = "IPA.log"
print(slFilePath)
is_wivi35 = ICSFileInterfaceLibrary.is_running_on_wivi_server() and os.path.splitext(sys.argv[1])[1].lower() == '.json'

if is_wivi35:	
	config = json.loads(slFilePath)
	ipaInstanceConfig = json.load(open(sys.argv[1]))
	config['output_dir'] = ipaInstanceConfig['output_dir']
	loggingPath = os.path.join(ipaInstanceConfig["output_dir"], "IPA.log")
	OutputFilePath = config['output_dir']
	OutputFilename = "vsbFileInfoSummary_" + str(ReportGenTimeStamp) + ".xlsx"
	OutputFilenameAndPath = os.path.join(OutputFilePath ,OutputFilename)
else:
	configFile = open(slFilePath)
	config = json.load(configFile)
	config['output_dir'] = os.getcwd()
	OutputFilePath = config['output_dir']
	OutputFilename = "vsbFileInfoSummary_" + str(ReportGenTimeStamp) + ".xlsx"
	OutputFilenameAndPath = os.path.join(OutputFilePath ,OutputFilename)

DeleteTempDB2FileAfterExecution = config['DeleteTempDB2FileAfterExecution']
log.info("Hello") 
log.info("Analyzing input files")

msg_Files = msgFiles(inputFilePaths, ReportGenTimeStamp, OutputFilePath, log)

with open(slFilePath) as configFile:
    config = json.load(configFile)

if not(math.isnan(config["StartTimeInSecondsFromStartOfFile"])) and (config["StartTimeAbsolute"] != ""):
	log.info("You can filter time by using StartTimeInSecondsFromStartOfFile or StartTimeAbsolute, not both")
	exit()

if (math.isnan(config["StartTimeInSecondsFromStartOfFile"])) and (config["StartTimeAbsolute"] == ""):
	log.info("You must either provide a value for StartTimeInSecondsFromStartOfFile or StartTimeAbsolute")
	exit()

MsgIDsPerNetwork = config["MsgIDsPerNetwork"]
for i in range(len(MsgIDsPerNetwork)):
	MsgIDsPerNetwork[i]["network_ID"] = ConvertNetworkStringToID(MsgIDsPerNetwork[i]["network_IDString"])
NumberOfNetworks = len(MsgIDsPerNetwork)
NetworkAndMsgIDQueryString = ""
tempMsgStringInDec= ""

#note that WriteFilteredVsb() puts in the "Select * from RawMessageData where" for you so you don't need to put it in your filter string

NetworkAndMsgIDQueryString = ""
for i in range(len(MsgIDsPerNetwork)):
	MsgIDsWithoutRegularExpressions = []
	MsgIDsWithRegularExpressions = []
	tempMsgList = str(MsgIDsPerNetwork[i]["MsgIdList"]).split(",")
	tempMsgStringInDec = ""
	for j in range(len(tempMsgList)): 
		if ((tempMsgList[j].find("*") !=- 1) or (tempMsgList[j].find("?") !=- 1)):
			MsgIDsWithRegularExpressions.append(tempMsgList[j].strip())
		else:
			MsgIDsWithoutRegularExpressions.append(tempMsgList[j].strip())		

	if (len(MsgIDsWithoutRegularExpressions)) > 0:
		NetworkAndMsgIDQueryString += "(NetworkId IN (" + str(MsgIDsPerNetwork[i]["network_ID"]) + ") AND Id IN ("
		for j in range(len(MsgIDsWithoutRegularExpressions)): 
			tempMsgInt = int(MsgIDsWithoutRegularExpressions[j], 16)
			tempMsgStringInDec += str(tempMsgInt)
			if(j < len(MsgIDsWithoutRegularExpressions) - 1):
				tempMsgStringInDec += ", "
		NetworkAndMsgIDQueryString += tempMsgStringInDec + "))"
		if(i < (len(MsgIDsPerNetwork)-1)) or len(MsgIDsWithRegularExpressions) > 0:
			NetworkAndMsgIDQueryString += " OR "

	#if any of the MsgIDs have wildcards, add them to the expression seperately
	if len(MsgIDsWithRegularExpressions) > 0:
		NetworkAndMsgIDQueryString += "(NetworkId IN (" + str(MsgIDsPerNetwork[i]["network_ID"]) + ") AND ("
		for k in range(len(MsgIDsWithRegularExpressions)):
			if (MsgIDsWithRegularExpressions[k].find("*") !=- 1):
				MsgIDsWithRegularExpressions[k] = MsgIDsWithRegularExpressions[k].replace("*", "%")
			if (MsgIDsWithRegularExpressions[k].find("?") !=- 1):
				MsgIDsWithRegularExpressions[k] = MsgIDsWithRegularExpressions[k].replace("?", "_")
			NetworkAndMsgIDQueryString += "(printf('%x', Id) like '" + MsgIDsWithRegularExpressions[k][2:len(MsgIDsWithRegularExpressions[k])] + "')"
			if(k < (len(MsgIDsWithRegularExpressions)-1)):
				NetworkAndMsgIDQueryString += " OR "
		NetworkAndMsgIDQueryString += ")"
		if(i < (len(MsgIDsPerNetwork)-1)):
			NetworkAndMsgIDQueryString += ") OR "
		else:
			NetworkAndMsgIDQueryString += ")"


# now go through each file and split using NetworkAndMsgIDQueryString
for msg_File in msg_Files.FilesListSorted:
	try:
		msg_File.FilteredVSBFilename = os.path.splitext(msg_File.InputFilePath + msg_File.InputFileName)[0] + "_" + ReportGenTimeStamp + "_Filtrd.vsb"
		# Open the message database
		
		log.info("Get info from DB file")
		conn = sqlite3.connect(msg_File.DB_FileName, timeout=10)
		conn.text_factory = lambda x: str(x, 'utf-8', 'ignore')

		# Read the file start and end timestamp from the Networks table
		cursor = conn.cursor()
		try:
			cursor.execute("SELECT Min(FirstTime), Max(LastTime) FROM Network")

			row = cursor.fetchone()
		
			if row is not None:
				startTimeAbsoluteNumeric = row[0]
				startTimeAbsoluteString = datetime.fromtimestamp(startTimeAbsoluteNumeric / 1e9, timezone.utc).isoformat()
				endTimeAbsoluteNumeric = row[1]
				endTimeAbsoluteString = datetime.fromtimestamp(endTimeAbsoluteNumeric / 1e9, timezone.utc).isoformat()
				if not(math.isnan(config["StartTimeInSecondsFromStartOfFile"])):
					FilteredStartTime = str(int(startTimeAbsoluteNumeric + config["StartTimeInSecondsFromStartOfFile"] * 1e9)) 
					FilteredEndTime = str(int(endTimeAbsoluteNumeric + config["EndTimeInSecondsFromStartOfFile"] * 1e9)) 
				else:
					FilteredStartTime = str(int(datetime.fromisoformat(config["StartTimeAbsolute"]).timestamp() * 1e9)) 
					FilteredEndTime = str(int(datetime.fromisoformat(config["EndTimeAbsolute"]).timestamp() * 1e9)) 
				if len(MsgIDsPerNetwork) > 0:
					TimeFilterString = " AND ((MessageTime > " + FilteredStartTime + ") AND (MessageTime < " + FilteredEndTime + "))"
				else:
					TimeFilterString = "((MessageTime > " + FilteredStartTime + ") AND (MessageTime < " + FilteredEndTime + "))"

			log.info("Generate filtered vsb file")
			if len(NetworkAndMsgIDQueryString) > 0:
				TempQueryString = "(" + NetworkAndMsgIDQueryString + ")"
			else:
				TempQueryString = ""

			msg_File.NumberOfRecordsInFilteredVSB = vsb.WriteFilteredVsb(msg_File.DB_FileName, msg_File.FilteredVSBFilename, TempQueryString + TimeFilterString, None)
			if msg_File.NumberOfRecordsInFilteredVSB > 0:
				sys.stdout.write("VSB file was created! with " + str(msg_File.NumberOfRecordsInFilteredVSB) + " in it. \n")
				log.info("Finished generating filtered vsb file")
			else:
				sys.stdout.write('VSB file has no records for output.\n')
				log.info('VSB file has no records for output.')

		except sqlite3.Error as er:
			log.info("Error in sqlite file" + msg_File.InputFileName)

		conn.close()
		if ( os.path.isfile(msg_File.DB_FileName) and (msg_File.FileCreatedByClass) \
		    and (config["DeleteTempDB2FileAfterExecution"] == "TRUE")  ):
			os.remove(msg_File.DB_FileName)


	except ValueError as e:
		log.info("Error message: " + str(e))
		print(str(e))


OutputDatabaseFilename = os.path.join(OutputFilePath, "CombinedNetworkFile_" + ReportGenTimeStamp + "_Filtrd.db2")
if ICSFileInterfaceLibrary.is_running_on_wivi_server():
	log.info("Running in wivi mode...")
	OutputVSBFilename = "CombinedNetworkFile_" + str(ReportGenTimeStamp) + "_Filtrd.vsb"
else:
	OutputVSBFilename = os.path.join(OutputFilePath, "CombinedNetworkFile_" + ReportGenTimeStamp + "_Filtrd.vsb")


FirstFileAdded = False
if config["CombineResultingFilesToASingleVSB"] == "TRUE":
	for msg_File in msg_Files.FilesListSorted:
		if msg_File.NumberOfRecordsInFilteredVSB > 0:
			if FirstFileAdded == False:
				vsb.CreateDatabase(msg_File.FilteredVSBFilename, OutputDatabaseFilename, None)
				FirstFileAdded = True
			else:
				vsb.AddToDatabase(msg_File.FilteredVSBFilename, OutputDatabaseFilename, None)			
			if os.path.isfile(msg_File.FilteredVSBFilename):
				os.remove(msg_File.FilteredVSBFilename)

	SortByTimeQueryString = "NetworkId >= 0 ORDER BY MessageTime"
	NumberOfRecordsInCombinedOutputFile = vsb.WriteFilteredVsb(OutputDatabaseFilename, OutputVSBFilename, SortByTimeQueryString, None)
	log.info("NumberOfRecordsInCombinedOutputFile = " + str(NumberOfRecordsInCombinedOutputFile))
	if NumberOfRecordsInCombinedOutputFile > 0:
		log.info("Finished generating combined filtered vsb file with " + str(NumberOfRecordsInCombinedOutputFile) + " records in file.")
	else:
		log.info('Combined vsb file has no records for output.')
	if ( os.path.isfile(OutputDatabaseFilename) and (config["DeleteTempDB2FileAfterExecution"] == "TRUE")  ):
		os.remove(OutputDatabaseFilename)
		log.info("Removed file: " + OutputDatabaseFilename )
else:
	if ICSFileInterfaceLibrary.is_running_on_wivi_server():
		VSBOutputZipFilename = "AllOutputVsbFiles_" + ReportGenTimeStamp + ".zip"
	else:
		VSBOutputZipFilename = os.path.join(OutputFilePath, "AllOutputVsbFiles_" + ReportGenTimeStamp + ".zip")	
	zipf = zipfile.ZipFile(VSBOutputZipFilename, 'w', zipfile.ZIP_DEFLATED)
	for msg_File in msg_Files.FilesListSorted:
		if (msg_File.NumberOfRecordsInFilteredVSB > 0) and (os.path.isfile(msg_File.FilteredVSBFilename)):
			zipf.write(msg_File.FilteredVSBFilename, arcname = os.path.basename(msg_File.FilteredVSBFilename))
			os.remove(msg_File.FilteredVSBFilename) 
			log.info("Removed file: " + msg_File.FilteredVSBFilename )

	zipf.close()
	 
if config["DeleteTempDB2FileAfterExecution"] == "FALSE":
	if ICSFileInterfaceLibrary.is_running_on_wivi_server():
		DB2OutputZipFilename = "AllOutputDB2Files_" + ReportGenTimeStamp + ".zip"
	else:
		DB2OutputZipFilename = os.path.join(OutputFilePath, "AllOutputDB2Files_" + ReportGenTimeStamp + ".zip")

	zipf = zipfile.ZipFile(DB2OutputZipFilename, 'w', zipfile.ZIP_DEFLATED)
	for msg_File in msg_Files.FilesListSorted:
		if ( (msg_File.NumberOfRecordsInFilteredVSB > 0) and os.path.isfile(msg_File.DB_FileName) and (msg_File.FileCreatedByClass)):
			zipf.write(msg_File.DB_FileName, arcname = os.path.basename(msg_File.DB_FileName))
			os.remove(msg_File.DB_FileName) 
			log.info("Removed Temp DB File")
	zipf.close()

log.info("Goodbye")