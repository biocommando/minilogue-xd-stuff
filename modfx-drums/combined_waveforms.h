#pragma once
#include <stdint.h>
const uint16_t *get_waveform(int id, uint16_t * length);

#define NUM_WAVEFORMS 12

// Waveform indices

#define WAVEFORM_ID_bd 0
#define WAVEFORM_ID_sd 1
#define WAVEFORM_ID_hhc 2
#define WAVEFORM_ID_rim 3
