#include "usermodfx.h"
#include "moog_filter_int.h"

static MicrotrackerMoog filter;

#define OVERSAMPLING 4

void MODFX_INIT(uint32_t platform, uint32_t api)
{
    (void) platform;
    (void) api;
    init_MicrotrackerMoog(&filter, 48000 * OVERSAMPLING);
}

void MODFX_PROCESS(const float *main_xn, float *main_yn, const float *sub_xn, float *sub_yn, uint32_t frames)
{
    (void) sub_xn;
    (void) sub_yn;

    const float *__restrict x = (const float *) main_xn;
    float *__restrict y = (float *) main_yn;

    for (uint32_t i = 0; i < frames; i++)
    {
        float input = *x;

        if (input > 1) input = 1;
        else if (input < -1) input = -1;

        int16_t output_i16;
        int16_t input_i16 = 0x7FFF * input;
        for (int oi = 0; oi < OVERSAMPLING; oi++)
        {
            // in its openest state, the filter cuts everything off ~15 kHz,
            // so the output is already band-limited
            output_i16 = MicrotrackerMoog_calculate(&filter, input_i16);
            input_i16 = 0;
        }
        float output = output_i16 / (float)0x7FFF;
        output *= OVERSAMPLING;
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
        MicrotrackerMoog_setResonance(&filter, v);
    }
    else if (index == k_user_modfx_param_depth)
    {
        MicrotrackerMoog_setCutoff(&filter, v);
    }
}
