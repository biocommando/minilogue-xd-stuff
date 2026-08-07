#pragma once

#include "userosc.h"

void OSC_INIT(uint32_t platform, uint32_t api);
void OSC_CYCLE(const user_osc_param_t *const params,
               int32_t *yn,
               const uint32_t frames);
void OSC_NOTEON(const user_osc_param_t *const params);
void OSC_NOTEOFF(const user_osc_param_t *const params);
void OSC_PARAM(uint16_t index, uint16_t value);