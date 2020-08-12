# A .mod file is just a list of hex digit pairs, with no comments. This example will overwrite 3 addresses. 
# If we assume 0x00530000 is the image base, and 0x0060fad8 is an RVA to the address holding the value for game volume.
# The first line will overwrite the first four bytes of address 0x00b3fad8 with 3e800000, changing volume to the float value 0.25.
0060fad8 3e800000
0000b003 f8ff
001f4e48 9090909090
