#pragma once
#include <stdint.h>

#define DATA_SAMPLERATE 16000

struct waveform_data
{
    const int8_t *data;
    uint16_t length;
};

struct waveform_data get_kick_waveform();
struct waveform_data get_snare_waveform();
struct waveform_data get_rim_waveform();
struct waveform_data get_hat_waveform();
// Alternative drum kit
struct waveform_data get_alt_kick_waveform();
struct waveform_data get_alt_snare_waveform();
struct waveform_data get_alt_rim_waveform();
struct waveform_data get_alt_hat_waveform();

// Waveform indices

#define WAVEFORM_ID_bd 0
#define WAVEFORM_ID_sd 1
#define WAVEFORM_ID_hhc 2
#define WAVEFORM_ID_rim 3
