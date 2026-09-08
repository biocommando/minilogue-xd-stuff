#pragma once
#include <stdint.h>

struct waveform {
    const uint16_t *data;
    uint16_t length;
};

struct waveform get_waveform(int id);

#define NUM_WAVEFORMS 12

// Waveform indices

#define WAVEFORM_ID_bd 0
#define WAVEFORM_ID_sd 1
#define WAVEFORM_ID_hhc 2
#define WAVEFORM_ID_rim 3
