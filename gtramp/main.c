#include "usermodfx.h"

#include "svf.h"

static float gain1, gain2, inv, gaincomp, tone;
static uint8_t amp_onoff;

static struct svf_state amp_svf[5];

__fast_inline float fast_tanh(float x)
{
    const float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}
__fast_inline float fast_tanh_capped(float x)
{
    if (x < -3.0f) return -1.0f;
    if (x > 3.0f) return 1.0f;
    return fast_tanh(x);
}

void MODFX_INIT(uint32_t platform, uint32_t api)
{
    (void) platform;
    (void) api;
    init_svf(&amp_svf[0], 117, 48000, 5.58);
    init_svf(&amp_svf[1], 465, 48000, 1.23);
    init_svf(&amp_svf[2], 1620, 48000, 1.38);
    init_svf(&amp_svf[3], 3930, 48000, 2.5);
    init_svf(&amp_svf[4], 5400, 48000, 1);
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

        input = fast_tanh(input * gain1);
        if (input > inv || input < -inv) input *= -1;
        input = fast_tanh_capped(input * gain2);
        float output = input;

        if (amp_onoff)
        {
            process_svf(&amp_svf[0], output);
            output = amp_svf[0].hp;
            process_svf(&amp_svf[1], output);
            output = output - tone * 0.567f * amp_svf[1].bp;
            process_svf(&amp_svf[2], output);
            output = output + tone * 1.291f * amp_svf[2].bp;
            process_svf(&amp_svf[3], output);
            output = amp_svf[3].lp;
            process_svf(&amp_svf[4], output);
            output = amp_svf[4].lp;
        }

        output *= gaincomp;
        *(y++) = output;
        *(y++) = output;
        x += 2;
    }
}

void MODFX_PARAM(uint8_t index, int32_t value)
{
    static float comp1, comp2;
    const float v = q31_to_f32(value);
    if (index == k_user_modfx_param_time)
    {
        gain1 = 1 + 30 * v;
        gain2 = 1 + 5 * v;
        inv = 2 - v;
        comp1 = 0.072f * (1 + v) / fast_tanh_capped(0.072f + 3.6f * v);
    }
    else if (index == k_user_modfx_param_depth)
    {
        amp_onoff = v > 0.01f;
        tone = v * 1.2f;
        amp_svf[3].q = 1.0f / (1 + 3 * v);
        if (amp_onoff)
            comp2 = 1.0f / (0.863f + 1.766f * v);
        else
            comp2 = 1;
    }
    gaincomp = comp1 * comp2;
}
