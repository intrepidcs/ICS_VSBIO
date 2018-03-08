# ICS_VSBIO
This module is designed to help users read Intrepid Control Systems VSB file. The file consists of messages that have been logged on the network. 

## Instillation 
The module can be installed using ```pip``` or can simply be installed from the git repo directly. 

### Pip Instillation

To install with ```pip``` simply enter the following in the terminal. 
```
pip install ICS_VSBIO
```

## Usage

The module contains two Classes ```VSBRead``` and ```VSBWrite```.  

### Reading From File

```
from VSBIO import VSBReader  as reader

vsbread = reader.VSBReader("example.vsb")
val = vsbread.read_next_message()

if val["status"] == reader.ReadStatus.eSuccess:
	print(val["Msg"].Protocol)
	print(val["Msg"].NetworkID)
	if (val["Msg"].ExtraDataPtr):
		print(val["EDP"])
		import binascii
		print( binascii.hexlify(val["EDP"]))
```
