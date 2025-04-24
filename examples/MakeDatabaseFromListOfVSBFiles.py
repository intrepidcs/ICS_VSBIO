import numpy as np
import sys
import os   
import logging
import json
import sys, os

from MsgFileClass import msgFiles
from ICS_VSBIO import VSBIOInterface as vsb
from ICS_VSBIO import ICSFileInterfaceLibrary
from shutil import copyfile
from zipfile import ZipFile
from datetime import datetime, timezone

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
	TemplateFilenameAndPath = os.path.join(OutputFilePath, config["TemplateFilename"])
else:
	configFile = open(slFilePath)
	config = json.load(configFile)
	config['output_dir'] = os.getcwd()
	OutputFilePath = config['output_dir']

#VSDBFilenameAndPath = config['VSDBFilenameAndPath']
handler = logging.FileHandler(loggingPath)
handler.setLevel(logging.INFO)
# create a logging format
formatter = logging.Formatter('%(asctime)s - %(name)s - %(message)s')
handler.setFormatter(formatter)
log.addHandler(handler)
log.info("Hello")

log.info(slFilePath)

# now go through each file, look for hits and log hits to dsr file
# create a list for vsbFiles

#now write the vsbFiles info to the template Excel Spreadsheet
log.info("Converting files to db")
FileTypesList = config["FileTypesList"]

for inputFilePath in inputFilePaths:
	InputFileName = os.path.basename(inputFilePath["path"])
	InputFilePath = os.path.dirname(inputFilePath["path"])
	filename, fileExtension = os.path.splitext(str(InputFileName))
	DB2_FileName = os.path.join(OutputFilePath, filename + "_" + ReportGenTimeStamp + ".db2") 
	PCAP_FileName = os.path.join(OutputFilePath, filename + "_" + ReportGenTimeStamp + ".pcap") 
	PCAPNG_FileName = os.path.join(OutputFilePath, filename + "_" + ReportGenTimeStamp + ".pcapng") 
	FileExtension = fileExtension
	FileCreatedByClass = False
	FileStartTime = ""
	FileEndTime = ""
	FileStartTimeRaw = 0
	FileEndTimeRaw = 0
	TempVSBExtracted = False
	zip2FolderName = ""

	log.info("InputFileName: " + str(InputFileName))
	#now compress output files
	if (fileExtension == ".zip"):
		with zipfile.ZipFile(os.path.join(InputFilePath, InputFileName), 'r') as zipObj:
			# Get a list of all archived file names from the zip
			listOfFileNames = zipObj.namelist()
			# Iterate over the file names
			log.info("First level zip filenames:")

			for zipfileName in listOfFileNames:
				log.info(zipfileName)

				# skip directories
				if not zipfileName:
					continue

				if zipfileName[len(zipfileName)-4:len(zipfileName)] == ('.vsb'):
					zipObj.extract(zipfileName, OutputFilePath)
					InputFileName = zipfileName
					FileExtension = ".vsb"
					InputFilePath = OutputFilePath
					TempVSBExtracted = True
					log.info("Only one level of zip")

				elif zipfileName[len(zipfileName)-8:len(zipfileName)] == ('.vsb.zip'):
					log.info(".vsb.zip file found")
					zip2FolderName = os.path.join(OutputFilePath.replace('\\', '/'),'tempZipFolderName').replace('\\', '/')
					zipObj.extract(zipfileName, zip2FolderName)
					zip3FileName = os.path.join(zip2FolderName,zipfileName).replace('\\', '/')
					with zipfile.ZipFile(zip3FileName) as zipObj2:
						zipObj2.extract(zipObj2.namelist()[0], OutputFilePath)
						InputFileName = zipObj2.namelist()[0]
						log.info(str(InputFileName))
						filename2, fileExtension2 = os.path.splitext(InputFileName)
						FileExtension = fileExtension2
						InputFilePath = OutputFilePath
						TempVSBExtracted = True
					try:
						shutil.rmtree(zip2FolderName)
						log.info("Removed top level zip file")
						
					except OSError as e:
						a=a

	if (FileExtension == ".vsb"):
		try:
			# Open the message database
			log.info("Create database for file " + str(os.path.join(InputFilePath, InputFileName)))
			for a in range(len(FileTypesList)):
				if FileTypesList[a] == "db2":
					CreateDB2Result = vsb.CreateDatabase(os.path.join(InputFilePath, InputFileName), DB2_FileName, None)
					log.info("Result of DB2 creation was: " + str(CreateDB2Result))
				elif FileTypesList[a] == "pcap":
					CreatePCAPResult = vsb.CreateDatabase(os.path.join(InputFilePath, InputFileName), PCAP_FileName, None)
					log.info("Result of pcap creation was: " + str(CreatePCAPResult))
				elif FileTypesList[a] == "pcapng":
					CreatePCAPNGResult = vsb.CreateDatabase(os.path.join(InputFilePath, InputFileName), PCAPNG_FileName, None)
					log.info("Result of pcapng creation was: " + str(CreatePCAPNGResult))
			
			"""
			OutputFilename = os.path.basename(DB2_FileName) + ".db"
			OutputFilenameAndPath = os.path.join(OutputFilePath ,OutputFilename)	
			ResultOfAddDecodings = vsb.AddVsdbDecodings(VSDBFilenameAndPath, DB2_FileName)
			log.info("Finished adding vsdb decodings to message db2 file")
			log.info("ResultOfAddDecodings = " + str(ResultOfAddDecodings))
			log.info("Starting to make signal db file")
			ResultOfCreateSignalDatabase = vsb.CreateSignalDatabase(DB2_FileName, OutputFilenameAndPath)
			log.info("ResultOfCreateSignalDatabase = " + str(ResultOfCreateSignalDatabase))
			log.info("Finished making signal db file")
			"""
		except ValueError as e:
			print(str(e))

log.info("Goodbye")
