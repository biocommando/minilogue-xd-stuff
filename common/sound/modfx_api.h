#pragma once

#include "usermodfx.h"

void MODFX_INIT(uint32_t platform, uint32_t api);
void MODFX_PROCESS(const float *main_xn, float *main_yn, const float *sub_xn, float *sub_yn, uint32_t frames);
void MODFX_PARAM(uint8_t index, int32_t value);
