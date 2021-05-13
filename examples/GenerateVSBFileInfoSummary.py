##########################################################################################################################################################################################################################################
#    Script Description:
#        This script opens a list of network message data files (vsb or db2) and makes a spreadsheet with summary information about each file listing the first and last timestamp
#         as well as a list of all the record count for each ArbID on each network
#
###########################################################################################################################################################################################################################################
#    Script Inputs: When you run this script you will be prompted with 2 file open dialog windows. The first asks for a config file with extension *.asl; the second asks
#    for a list of one or more network data files (*.vsb, *.db2)
#
#         Script config file GenerateFileInfoSummary_Config.asl is a JSON file used to configure the script. This file has the following keys:
#
#             TemplateFilename - Excel file that is used as a template for generating the report. You can change the colors or bolding of the template
#								 and your changes will be used for future reports. 
#
#             DeleteTempDB2FileAfterExecution - Flag can be TRUE or FALSE. If TRUE it will delete any temporary db2 if they were made by the script. Any db2 files that were
#                                               input to the script will not be deleted
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
import openpyxl

from openpyxl.reader.excel import load_workbook
from UtilityFunctions import ConvertNetworkStringToID
from MsgFileClass import msgFiles
from ICS_IPA import DataFileIOLibrary as icsFI
from ICS_IPA import IPAInterfaceLibrary
from shutil import copyfile

from datetime import datetime, timezone
from ICS_IPA import IPAInterfaceLibrary

logging.basicConfig(level=logging.INFO)
log = logging.getLogger(__name__)
handler = logging.FileHandler('IPA.log')
handler.setLevel(logging.INFO)

# create a logging format
formatter = logging.Formatter('%(asctime)s - %(name)s - %(message)s')
handler.setFormatter(formatter)
log.addHandler(handler)

log.info("Hello")

slFilePath = IPAInterfaceLibrary.get_config_file()
inputFilePaths = IPAInterfaceLibrary.get_input_file_list()

#  C:\Jmitchell\FordExplorerData\extracted data 13-03-2021 02-57-29-510938 PM\Script 15-03-2021 08-55-07-650000 AM

log.info(slFilePath)

with open(slFilePath) as configFile:
    config = json.load(configFile)

# now go through each file, look for hits and log hits to dsr file
# create a list for vsbFiles
ReportGenTimeStamp = datetime.now().strftime("%m-%d-%y_%H-%M-%S")
log.info("Analyzing input files")
input_msg_Files = msgFiles(inputFilePaths, ReportGenTimeStamp)

if IPAInterfaceLibrary.is_running_on_wivi_server():
	OutputFilePath = os.path.dirname(sys.argv[0]) + "\\"
	TemplateFilenameAndPath = OutputFilePath + config["TemplateFilename"]
	OutputFilename = "vsbFileInfoSummary_" + str(ReportGenTimeStamp) + ".xlsx"
	OutputFilenameAndPath = OutputFilePath + OutputFilename
else:
	OutputFilePath = os.path.dirname(sys.argv[0]) + "\\"
	TemplateFilenameAndPath = OutputFilePath + config["TemplateFilename"]
	OutputFilename = "vsbFileInfoSummary_" + str(ReportGenTimeStamp) + ".xlsx"
	OutputFilenameAndPath = OutputFilePath + OutputFilename

#now write the vsbFiles info to the template Excel Spreadsheet
log.info("Creating Excel output file")
wb = load_workbook(TemplateFilenameAndPath)	
DataEntryRow = 3

if(len(input_msg_Files.FilesListSorted) > 0):	
	if not('vsbStatSummary' in wb.sheetnames):
		log.info(OutputFilename + " doesn't have a sheet named vsbStatSummary")
		wb.save(OutputFilenameAndPath)
		wb.close()
		quit()
	ws = wb.get_sheet_by_name('vsbStatSummary')

	#Read in the header rows 
	ColumnHeadersFromTemplateSpreadsheet = []
	i = 0
	while (ws.cell(row=2, column=1 + i).value != None):
		ColumnHeadersFromTemplateSpreadsheet.append(str(ws.cell(row=2, column=1 + i).value))
		i = i + 1
	NumberOfDIDColumnsInLog = i-1

	log.info("Begin writing report data to DTCReport sheet")

	for a in range(len(input_msg_Files.FilesListSorted)):
		log.info("Writing network info for File number " + str(a))
		for b in range(len(input_msg_Files.FilesListSorted[a].FileneNetworks)):
			for c in range(len(input_msg_Files.FilesListSorted[a].FileneNetworks[b].network_msg_ids_hex)):
				ws.cell(row=DataEntryRow, column=1, value=input_msg_Files.FilesListSorted[a].FileStartTime)
				ws.cell(row=DataEntryRow, column=2, value=input_msg_Files.FilesListSorted[a].FileEndTime)
				ws.cell(row=DataEntryRow, column=3, value=input_msg_Files.FilesListSorted[a].FileName)
				ws.cell(row=DataEntryRow, column=4, value=input_msg_Files.FilesListSorted[a].FileneNetworks[b].network_name)
				ws.cell(row=DataEntryRow, column=5, value=input_msg_Files.FilesListSorted[a].FileneNetworks[b].network_msg_ids_hex[c])
				ws.cell(row=DataEntryRow, column=6, value=input_msg_Files.FilesListSorted[a].FileneNetworks[b].network_msg_num_msgs[c])
				DataEntryRow = DataEntryRow + 1

		if ( os.path.isfile(input_msg_Files.FilesListSorted[a].DB_FileName) and (input_msg_Files.FilesListSorted[a].FileCreatedByClass) \
		    and (config["DeleteTempDB2FileAfterExecution"] == "TRUE")  ):
			os.remove(input_msg_Files.FilesListSorted[a].DB_FileName)

	log.info("Saving report file")
	
	wb.save(OutputFilenameAndPath)
	wb.close()

	log.info("Goodbye")

