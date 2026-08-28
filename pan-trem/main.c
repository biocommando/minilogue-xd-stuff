#include "usermodfx.h"

static float osc_ph_offset, osc_phase, osc_ph_inc, mod_gain;

inline static float tri1(float phase)
{
    if (phase < 0.5)
        return 4 * phase - 1;
    else
        return -4 * phase + 3;
}

#define LIMIT_PHASE(phase) ((phase)-(int)(phase))

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
        osc_phase = LIMIT_PHASE(osc_phase + osc_ph_inc);

        *(y++) = *(x++) * (1 - mod_gain * tri1(osc_phase));
        *(y++) = *(x++) * (1 - mod_gain * tri1(LIMIT_PHASE(osc_phase + osc_ph_offset)));
    }
}

void MODFX_PARAM(uint8_t index, int32_t value)
{
    const float v = q31_to_f32(value);
    if (index == k_user_modfx_param_time)
    {
        osc_ph_inc = v * 10.0 / 48000.0;
    }
    else if (index == k_user_modfx_param_depth)
    {
        mod_gain = (v - 0.5) * 2;
        osc_ph_offset = v * 0.5;
    }
}
