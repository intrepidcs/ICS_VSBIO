from ICS_VSBIO import VSBReader  as reader
from ICS_VSBIO import VSBWriter  as writer
from ICS_VSBIO import VSBIOFlags as flags
import binascii

vsbRead = reader.VSBReader("input.vsb")
vsbWrite = writer.VSBWriter("ouput.vsb")

count = 0
print('start')
try:
	for msg in vsbRead:
		count += 1
		if not count % 2000:
			print('{0}% of file read'.format(vsbRead.get_progress()))
		if msg.info.NetworkID == flags.NETID_HSCAN:
			if (msg.info.ExtraDataPtr):
				print(binascii.hexlify(msg.EDP))
			vsbWrite.write_msg(msg)
			
except ValueError as e:
	print(str(e))
except Exception as ex:
	print(ex)
else:
	print('Success')


