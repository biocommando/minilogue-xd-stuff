#pragma once

#include <stdint.h>

float get_semitone_ratio(uint16_t shape);

float calculate_drift(uint32_t * accumulator, uint32_t frames);
