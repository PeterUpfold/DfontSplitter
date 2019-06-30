# Split TTC files


header = Read 4 bytes at beginning of file

if header != "ttcf"
	bail out

version = Read next 4 bytes

if version != 0x00
