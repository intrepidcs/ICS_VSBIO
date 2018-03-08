from VSBIO import VSBReader  as reader
from VSBIO import VSBWriter  as writer

vsbread = reader.VSBReader("example.vsb")
val = vsbread.read_next_message()

print(val["Msg"].Protocol)
print(val["Msg"].NetworkID)
if (val["Msg"].ExtraDataPtr):
	print(val["EDP"])
	import binascii
	print( binascii.hexlify(val["EDP"]))

vsbwrite = writer.VSBWriter("example2.vsb")
vsbwrite.write_msg(val)
