# #############################################################################
# Project Customization
# #############################################################################

PROJECT = oscstack

UCSRC = osc.c ../common/src/basic_oscillator.c ../common/src/synth_random.c wavetable.c

UCXXSRC = 

UINCDIR = ../common/src

UDEFS = -DBASIC_OSCILLATOR_SINE_TABLE_SIZE=512 

ULIB = 

ULIBDIR =

