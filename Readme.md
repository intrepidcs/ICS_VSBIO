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

```py
from ICS_VSBIO import VSBReader  as reader
from ICS_VSBIO import VSBWriter  as writer
from ICS_VSBIO import VSBIOFlags as flags
import binascii

vsbread = reader.VSBReader("input.vsb")
vsbwrite = writer.VSBWriter("ouput.vsb")

count = 0
print('start')
try:
    for message in vsbread:
        count += 1
        if not count % 2000:
            print('{0}% of file read'.format(vsbread.getProgress()))
        if message.info.NetworkID == flags.NETID_HSCAN:
            if (message.info.ExtraDataPtr):
                print(binascii.hexlify(message.exData))
            vsbwrite.write_msg(message)

except ValueError as e:
    print(str(e))
except:
    print('an unknown error has occurred')
else:
    print('Success')
```

#### VSBRead functions
```__init__``` Takes filename to initialize process 

```get_progress()``` returns the progress as a integer percentage

```get_error_message()``` return Error messages if any.

```get_display_message()``` return Display messages if any

```get_satus()``` returns the current state

```get_status_as_string()``` returns the current state in string format

#### VSBWrite functions
```__init__``` Takes filename to initialize process 

```write_msg(message)``` writes vsb message to file.  




