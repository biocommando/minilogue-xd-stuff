# #############################################################################
# Project Customization
# #############################################################################

COMMON = ../common/src

PROJECT = ms20filter

UCSRC = main.c $(COMMON)/ms20_filter.c $(COMMON)/adsr_envelope.c $(COMMON)/envelope_stage.c

UCXXSRC =

UINCDIR = $(COMMON)

UDEFS = 

ULIB = 

ULIBDIR =
