# Monad
Basic framework exposing Devil Dagger object values to developers/modders. Currently a POC.


Monad will read a file named modad.mod in the same directory to modify the runtime memory of Devil Daggers.
modad.mod is a text file with two DWORD-sized hexidecimal numbers separated by a space. 
The first DWORD is the RVA from the image base. The second is an AOB (in big-endian) used to patch the given address.
See the example modad.mod file.
