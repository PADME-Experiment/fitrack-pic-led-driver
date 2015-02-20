.PHONY: all


SDCC=sdcc-sdcc #$(HOME)/.opt/sdcc/svn/bin/sdcc
COPT="-I$(HOME)/sdcc/device/non-free/include/ -I$(HOME)/sdcc/device/include/ -L$(HOME)/sdcc/device/non-free/lib/ -L$(HOME)/sdcc/device/lib"

all: pic
	~/.opt/sdcc/svn/bin/sdcc -I ~/sdcc/device/non-free/include/  ~/.opt/sdcc/svn/lib64/libiberty.a pic.c
