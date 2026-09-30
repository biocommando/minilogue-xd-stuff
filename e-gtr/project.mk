# #############################################################################
# Project Customization
# #############################################################################

PROJECT = e-gtr

UCSRC = osc.c ../common/src/adsr_envelope.c ../common/src/basic_oscillator.c ../common/src/envelope_stage.c ../common/src/synth_random.c ../common/src/flt.c

UCXXSRC = 

UINCDIR = ../common/src

UDEFS = -DBASIC_OSCILLATOR_SINE_TABLE_SIZE=1 -DBASIC_OSCILLATOR_INTERPOLATED_WT -DBASIC_OSCILLATOR_NO_DC_FLT

ULIB = 

ULIBDIR =

