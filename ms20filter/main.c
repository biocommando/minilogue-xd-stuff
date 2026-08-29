#include "usermodfx.h"
#include "ms20_filter.h"

static MS20Filter filter;

void MODFX_INIT(uint32_t platform, uint32_t api)
{
    (void) platform;
    (void) api;
    MS20Filter_init(&filter, 48000);
}

void MODFX_PROCESS(const float *main_xn, float *main_yn, const float *sub_xn, float *sub_yn, uint32_t frames)
{
    (void) sub_xn;
    (void) sub_yn;

    const float *__restrict x = (const float *) main_xn;
    float *__restrict y = (float *) main_yn;

    for (uint32_t i = 0; i < frames; i++)
    {
        const float input = *x;
        const float output = MS20Filter_calculate(&filter, input);
        *(y++) = output;
        *(y++) = output;
        x += 2;
    }
}

void MODFX_PARAM(uint8_t index, int32_t value)
{
    const float v = q31_to_f32(value);
    if (index == k_user_modfx_param_time)
    {
        MS20Filter_setResonance(&filter, v);
    }
    else if (index == k_user_modfx_param_depth)
    {
        MS20Filter_setCutoff(&filter, v);
    }
}
