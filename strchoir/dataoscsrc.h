#pragma once
#include <stdint.h>

struct waveform_data
{
    const int8_t *data;
    uint16_t length;
};

struct waveform_data get_choir_waveform();
struct waveform_data get_string_waveform();
struct waveform_data get_piano_waveform();
struct waveform_data get_clockenspiel_waveform();
