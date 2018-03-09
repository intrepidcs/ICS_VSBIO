from ICS_VSBIO import VSBReader  as reader
from ICS_VSBIO import VSBWriter  as writer
from ICS_VSBIO import VSBIOFlags as flags
import binascii

vsbread = reader.VSBReader("input.vsb")
vsbwrite = writer.VSBWriter("ouput.vsb")

count = 0
print('start')
try:
	for msg in vsbread:
		count += 1
		if not count % 2000:
			print('{0}% of file read'.format(vsbread.getProgress()))
		if msg.Msg.NetworkID == flags.NETID_HSCAN:
			if (msg.Msg.ExtraDataPtr):
				print(binascii.hexlify(msg.EDP))
			vsbwrite.write_msg(msg)
			
except ValueError as e:
	print(str(e))
except:
	print('an unknown error has occurred')
else:
	print('Success')


