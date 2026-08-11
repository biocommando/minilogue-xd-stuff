#pragma once
#include <stdint.h>
const uint16_t *get_waveform(int id, uint16_t *length);

#define NUM_WAVEFORMS 12

// Waveform indices

#define WAVEFORM_ID_bd 0
#define WAVEFORM_ID_bd1 1
#define WAVEFORM_ID_sd 2
#define WAVEFORM_ID_sd1 3
#define WAVEFORM_ID_hcp 4
#define WAVEFORM_ID_hhc 5
#define WAVEFORM_ID_tam 6
#define WAVEFORM_ID_hho 7
#define WAVEFORM_ID_cow 8
#define WAVEFORM_ID_crs 9
#define WAVEFORM_ID_rim 10
#define WAVEFORM_ID_ht 11
