#include "usermodfx.h"

static float max_level, outlevel, freq, phase;

void MODFX_INIT(uint32_t platform, uint32_t api)
{
    (void) platform;
    (void) api;
}


void MODFX_PROCESS(const float *main_xn, float *main_yn, const float *sub_xn, float *sub_yn, uint32_t frames)
{
    (void) sub_xn;
    (void) sub_yn;

    const float *__restrict x = (const float *) main_xn;
    float *__restrict y = (float *) main_yn;

    for (uint32_t i = 0; i < frames; i++)
    {
        float s = *x;
        
        if (fabsf(s) > max_level)
        {
            max_level = fabsf(s);
            freq = max_level * 1000 / 48000;
        }
        phase += freq;
        phase -= (int)phase;
        s = fastsinfullf(2 * M_PI * phase) * outlevel;
        
        *(y++) = s;
        *(y++) = s;
        x += 2;
    }
}

void MODFX_PARAM(uint8_t index, int32_t value)
{
    const float v = q31_to_f32(value);
    if (index == k_user_modfx_param_time)
    {
        if (v < 0.1)
            max_level = 0;
    }
    else if (index == k_user_modfx_param_depth)
    {
        outlevel = v;
    }
}
