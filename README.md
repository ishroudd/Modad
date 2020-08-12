# Modad
monad is a c++ library that hooks into the memory space of Devil Daggers and offers a runtime read/write API.

Modad is a wrapper for the monad library to provide an easy interface for developers and modders to create distributable changes to object values in Devil Daggers.
A .mod file lists address RVAs and bytes to write to them. Running Modad with Devil Daggers running will read all .mod files in a ./mod subdirectory and adjust Devil Dagger's runtime addresses appropriately.

A .mod file is a simple text file that lists pairs of hexidecimal bytes, in "RVA BYTES" format. RVA is a DWORD-sized RVA from the image base. BYTES are the big-endian hexidecimal bytes to write. Mods shouldn't try to write more than 7 bytes per line (IE don't do "007CFA40 9090F00D). Writing memory can get buggy ATM, the less bytes you read/write the better. Note that alphabetical read order is important, b.mod can overwrite bytes previously written by a.mod.
