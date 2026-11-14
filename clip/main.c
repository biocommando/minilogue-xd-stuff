#include "usermodfx.h"
#include "svf.h"

static float gain, gain_comp;

static struct svf_state flt, flt_new;
uint8_t flt_on;

void MODFX_INIT(uint32_t platform, uint32_t api)
{
    (void) platform;
    (void) api;
    init_svf(&flt, 20, 48000, 0.5f);
}


void MODFX_PROCESS(const float *main_xn, float *main_yn, const float *sub_xn, float *sub_yn, uint32_t frames)
{
    (void) sub_xn;
    (void) sub_yn;

    float flt_f_delta = 0;
    if (flt_on)
    {
        flt_f_delta = (flt_new.f - flt.f) / frames;
    }

    const float *__restrict x = (const float *) main_xn;
    float *__restrict y = (float *) main_yn;

    for (uint32_t i = 0; i < frames; i++)
    {
        float s = *x;

        if (flt_on)
        {
            process_svf(&flt, s);
            s = flt.hp;
        }

        s *= gain;
        s = s < -1 ? -1 : (s > 1 ? 1 : s);

        s *= gain_comp;
        if (flt_on)
            s += flt.lp;
        *(y++) = s;
        *(y++) = s;
        x += 2;

        flt.f += flt_f_delta;
    }
}

void MODFX_PARAM(uint8_t index, int32_t value)
{
    const float v = q31_to_f32(value);
    if (index == k_user_modfx_param_time)
    {
        gain = 1 + 145 * v * v;
        gain_comp = 1.094204 + -10.038871*v + 39.380113*v*v + -72.136936*v*v*v + 62.255558*v*v*v*v + -20.432452*v*v*v*v*v;
    }
    else if (index == k_user_modfx_param_depth)
    {
        if (v < 0.01f)
            flt_on = 0;
        else
        {
            flt_on = 1;
            init_svf(&flt_new, 20 + v * 3000, 48000, 0.5f);
        }
    }
}
