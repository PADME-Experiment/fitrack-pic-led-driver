.PHONY: all clean up prep
.IGNORE: clean

.DEFAULT_GOAL:=all


PK2CMDINSTALLDIR=$(HOME)/Downloads/pk2cmdv1-20Linux2-6
SDCCINSTALLDIR=$(HOME)/.opt/sdcc/svn
SRCLIBDIR=$(SDCCINSTALLDIR)/share/sdcc/lib/src/
HEXFILES=pic16f88.hex
OBJLIBFILES=_atoi.o _strcpy.o _strlen.o _itoa.o


SDCC=sdcc #$(HOME)/.opt/sdcc/svn/bin/sdcc
SDCFLAGS=--use-non-free -mpic14 -p16f88



DISABLEFLAGS=--nooverlay --nogcse --nolabelopt --noinvariant --noinduction --nojtbound --noloopreverse         --no-peep-return --no-peep --opt-code-speed

#    --nooverlay          # Disable overlaying leaf function auto variables
#    --nogcse             # Disable the GCSE optimisation
#    --nolabelopt         # Disable label optimisation
#    --noinvariant        # Disable optimisation of invariants
#    --noinduction        # Disable loop variable induction
#    --nojtbound          # Don't generate boundary check for jump tables
#    --noloopreverse      # Disable the loop reverse optimisation
#    --no-peep            # Disable the peephole assembly file optimisation
#    --no-reg-params      # On some ports, disable passing some parameters in registers
#    --peep-asm           # Enable peephole optimization on inline assembly
#    --peep-return        # Enable peephole optimization for return instructions
#    --no-peep-return     # Disable peephole optimization for return instructions
#    --peep-file          # <file> use this extra peephole file
#    --opt-code-speed     # Optimize for code speed rather than size
#    --opt-code-size      # Optimize for code size rather than speed
#    --max-allocs-per-node#  Maximum number of register assignments considered at each node of the tree decomposition
#    --nolospre           # Disable lospre
#    --lospre-unsafe-read # Allow unsafe reads in lospre


PATH:=$(SDCCINSTALLDIR)/bin:$(PATH)
PATH:=$(PK2CMDINSTALLDIR):$(PATH)
CPATH:=$(SDCCINSTALLDIR)/share/sdcc/include/:$(SDCCINSTALLDIR)/share/sdcc/non-free/include/:$(CPATH)
LIBRARY_PATH:=$(SDCCINSTALLDIR)/lib64/:$(SDCCINSTALLDIR)/share/sdcc/lib/:$(SDCCINSTALLDIR)/share/sdcc/non-free/lib/:$(LIBRARY_PATH)
LD_LIBRARY_PATH:=/usr/lib/:$(LD_LIBRARY_PATH)  # pk2cmd


all:$(HEXFILES)

clean:
	rm *.asm *.cod *.hex *.lst *.o

prep:
	$(SDCC) $(SDCFLAGS) -E pic16f88.c

$(OBJLIBFILES): %.o: $(SRCLIBDIR)/%.c
	$(SDCC) $(SDCFLAGS) -c $< -o $@

$(HEXFILES):%.hex : %.c $(OBJLIBFILES)
	 $(SDCC) $(SDCFLAGS) $< $(OBJLIBFILES)
#	 $(SDCC) $(DISABLEFLAGS) $(SDCFLAGS) $< $(OBJLIBFILES)

up: pic16f88.hex
	pk2cmd -P -M -F$<

upX: pic16f88.hex
	pk2cmd -P PIC16F88 -X -M -F$<

upXE: pic16f88.hex
	pk2cmd -P PIC16F88 -X -E

down: pic16f88.hex
	pk2cmd -P -M -F$<
