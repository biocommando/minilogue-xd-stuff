#pragma once
#include <stdint.h>

float process_syn_drum();
void set_syn_drum_params(int drum_idx, float freq_modifier);
void set_syn_drum_variation(uint8_t new_variation);
