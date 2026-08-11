#include "usermodfx.h"

static float gain = 1, clip_lev0 = -1, clip_lev1 = 1, offs = 0;

void MODFX_INIT(uint32_t platform, uint32_t api)
{
    (void) platform;
    (void) api;
}

static inline float process_one_sample(float input)
{
    float s = input;
    s *= gain;
    if (s < clip_lev0)
        s = clip_lev0;
    if (s > clip_lev1)
        s = clip_lev1;
    s += offs;

    return s;
}

void MODFX_PROCESS(const float *main_xn, float *main_yn, const float *sub_xn, float *sub_yn, uint32_t frames)
{
    (void) sub_xn;
    (void) sub_yn;

    const float *__restrict x = (const float *) main_xn;
    float *__restrict y = (float *) main_yn;

    for (uint32_t i = 0; i < frames; i++)
    {
        for (int ch = 0; ch < 2; ch++)
        {
            float output = process_one_sample(*(x++));
            *(y++) = output;
        }
    }
}

void MODFX_PARAM(uint8_t index, int32_t value)
{
    const float v = q31_to_f32(value);
    if (index == k_user_modfx_param_time)
    {
        gain = 1 + 299 * v * v;
    }
    else if (index == k_user_modfx_param_depth)
    {
        clip_lev1 = v;
        offs = 1 - v;
    }
}
