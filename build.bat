windres ic.rc -O coff -o ic.res
windres info.rc -O coff -o info.res
g++ leafpack.cpp -o leafpack ic.res info.res -static