from ICS_VSBIO import VSBReader  as reader
from ICS_VSBIO import VSBWriter  as writer
from ICS_VSBIO import VSBIOFlags as flags

vsbread = reader.VSBReader("example.vsb")
vsbwrite = writer.VSBWriter("example2.vsb")

print('start')
while True:
	val = vsbread.read_next_message()
	if val["status"] == reader.ReadStatus.eSuccess:
		print('protocol: ', val["Msg"].Protocol, ' Network: ', val["Msg"].NetworkID )
		if val["Msg"].NetworkID == flags.NETID_HSCAN:
			if (val["Msg"].ExtraDataPtr):
				print(val["EDP"])
				import binascii
				print( binascii.hexlify(val["EDP"]))

			vsbwrite.write_msg(val)
			print('Network copied')
		else:
			print('network skipped')
	else:
		break;
print('end with a status of ', val["status"])
