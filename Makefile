all: version.dll

CXX      = g++
CC       = gcc
ASM      = nasm

CXXFLAGS = -O2 -std=c++17 -DNDEBUG -D_CONSOLE -Wfatal-errors
CFLAGS   = -O2
LDFLAGS  = -static-libgcc -static-libstdc++ -shared -lntdll


dllmain.o: RSAPatch/dllmain.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
exports.o: RSAPatch/exports.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
Utils.o: RSAPatch/Utils.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

version.o: RSAPatch/version.asm
	$(ASM) -f win64 $< -o $@

buffer.o: minhook/src/buffer.c
	$(CC) $(CFLAGS) -c $< -o $@
hook.o: minhook/src/hook.c
	$(CC) $(CFLAGS) -c $< -o $@
trampoline.o: minhook/src/trampoline.c
	$(CC) $(CFLAGS) -c $< -o $@
hde64.o: minhook/src/hde/hde64.c
	$(CC) $(CFLAGS) -c $< -o $@

version.dll: dllmain.o exports.o Utils.o buffer.o hook.o trampoline.o hde64.o version.o RSAPatch/Exports.def
	$(CXX) $^ $(LDFLAGS) -o $@

clean:
	rm -f *.o version.dll

rebuild: clean all

.PHONY: all clean rebuild