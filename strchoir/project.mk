# #############################################################################
# Project Customization
# #############################################################################

COMMON = ../common/src

PROJECT = StrChoir

WAVEFORMS = glockenspiel.c piano.c choir.c string.c

UCSRC = $(WAVEFORMS) osc.c $(COMMON)/flt.c $(COMMON)/synth_random.c

UCXXSRC =

UINCDIR = $(COMMON)

UDEFS =

ULIB = 

ULIBDIR =

