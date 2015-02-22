.PHONY: all clean
.IGNORE: clean

.DEFAULT_GOAL:=all


SDCCINSTALLDIR=$(HOME)/.opt/sdcc/svn
SRCLIBDIR=$(HOME)/sdcc/device/lib
HEXFILES=pic16f88.hex
OBJLIBFILES=_atoi.o _strcpy.o _strlen.o _itoa.o


SDCC=sdcc #$(HOME)/.opt/sdcc/svn/bin/sdcc
SDCFLAGS=--use-non-free -mpic14 -p16f88


PATH:=$(SDCCINSTALLDIR)/bin:$(PATH)
CPATH:=$(SDCCINSTALLDIR)/share/sdcc/include/:$(SDCCINSTALLDIR)/share/sdcc/non-free/include/:$(CPATH)
LIBRARY_PATH:=$(SDCCINSTALLDIR)/lib64/:$(SDCCINSTALLDIR)/share/sdcc/lib/:$(SDCCINSTALLDIR)/share/sdcc/non-free/lib/:$(LIBRARY_PATH)


all:$(HEXFILES)

clean:
	rm *.asm *.cod *.hex *.lst *.o

$(OBJLIBFILES): %.o: $(SRCLIBDIR)/%.c
	$(SDCC) $(SDCFLAGS) -c $< -o $@

$(HEXFILES):%.hex : %.c $(OBJLIBFILES)
	 $(SDCC) $(SDCFLAGS) $< $(OBJLIBFILES)
