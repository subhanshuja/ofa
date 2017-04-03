import base64
import struct
import sys

if len(sys.argv) < 2:
  print "Usage: patch_dat_file.py {DAT_FILE_TO_PATCH}";
  sys.exit(-1)

file_name = sys.argv[1];

# CHR-314 introduced additional field to serialized page state. Unfortunately
# this means that we need to patch binary test files whenever chrome add new
# version. The procedure to do this is:

# 1) Base64 decode the file
decoded =  base64.b64decode(open(file_name, 'r').read())

# 2) Add 130(4bytes little endian hexidecimal 0x82000000) before referrer for
#    which the pattern is is 24000000 68007400 74007000. It should occur twice
#    in the file.
pattern = "\x24\x00\x00\x00\x68\x00\x74\x00\x74\x00\x70\x00"
to_replace = "\x82\x00\x00\x00\x24\x00\x00\x00\x68\x00\x74\x00\x74\x00\x70\x00"
decoded_replaced = decoded.replace(pattern, to_replace)

# 3) Modify the payload length stored in the first 4 bytes of the file.
#    Again this is little endian.
length_change = len(decoded_replaced) - len(decoded);
original_lenght = struct.unpack("<I",decoded[0:4])[0]
modified_lenght_str = struct.pack("<I", original_lenght + length_change);
decoded_replaced = modified_lenght_str + decoded_replaced[4:]

# 4) Base64 encode the file.
encoded = base64.b64encode(decoded_replaced)

# 5) Store it divided to 77 char lines
output_file = open(file_name, 'w')

line_length = 77
i = 0
while i < len(encoded):
  output_file.write(encoded[i:i+line_length] + "\n")
  i += line_length

output_file.close()