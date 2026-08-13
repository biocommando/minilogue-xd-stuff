#pragma once

#include <stdint.h>

static inline int32_t safe_f32_to_q31(float f) {
	if (f >= 1.0f) return 0x7FFFFFFF;
	if (f < -1.0f) return 0x80000000;
	return ((int32_t)((float)(f) * (float)0x7FFFFFFF));
}

#define OSC_LOOP(var_name, yn, frames) \
	q31_t *__restrict var_name = (q31_t *) (yn); \
    const q31_t *var_name##_e = var_name + (frames); \
    for (; var_name != var_name##_e;)
