.PHONY: all clean up
.IGNORE: clean

.DEFAULT_GOAL:=all


PK2CMDINSTALLDIR=$(HOME)/Downloads/pk2cmdv1-20Linux2-6
SDCCINSTALLDIR=$(HOME)/.opt/sdcc/svn
SRCLIBDIR=$(HOME)/sdcc/device/lib
HEXFILES=pic16f88.hex
OBJLIBFILES=_atoi.o _strcpy.o _strlen.o _itoa.o


SDCC=sdcc #$(HOME)/.opt/sdcc/svn/bin/sdcc
SDCFLAGS=--use-non-free -mpic14 -p16f88


PATH:=$(SDCCINSTALLDIR)/bin:$(PATH)
PATH:=$(PK2CMDINSTALLDIR):$(PATH)
CPATH:=$(SDCCINSTALLDIR)/share/sdcc/include/:$(SDCCINSTALLDIR)/share/sdcc/non-free/include/:$(CPATH)
LIBRARY_PATH:=$(SDCCINSTALLDIR)/lib64/:$(SDCCINSTALLDIR)/share/sdcc/lib/:$(SDCCINSTALLDIR)/share/sdcc/non-free/lib/:$(LIBRARY_PATH)
LD_LIBRARY_PATH:=/usr/lib/:$(LD_LIBRARY_PATH)  # pk2cmd


all:$(HEXFILES)

clean:
	rm *.asm *.cod *.hex *.lst *.o

$(OBJLIBFILES): %.o: $(SRCLIBDIR)/%.c
	$(SDCC) $(SDCFLAGS) -c $< -o $@

$(HEXFILES):%.hex : %.c $(OBJLIBFILES)
	 $(SDCC) $(SDCFLAGS) $< $(OBJLIBFILES)

up: pic16f88.hex
	pk2cmd -P -M -F$<
