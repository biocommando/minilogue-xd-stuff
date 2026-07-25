#include "usermodfx.h"
#include "asymcompr.h"

#define SAMPLERATE 48000

static float gain = 1;
static float noise_gate_lev = 0.1;
int next_frame_silent = 0;
static AsymComprParams input_compr, output_compr;

void MODFX_INIT(uint32_t platform, uint32_t api)
{
    (void)platform;
    (void)api;
    input_compr.threshold0 = 0.6;
    input_compr.ratio0 = 0.7;
    input_compr.threshold1 = -0.7;
    input_compr.ratio1 = 0.6;
    
    output_compr.threshold0 = 0.9;
    output_compr.ratio0 = 0.8;
    output_compr.threshold1 = -0.8;
    output_compr.ratio1 = 0.9;
}

__fast_inline float fastsqrt(float input)
{
    // Square root approximation using Halley's method
	float xn = input < 0.1 ? 0.056 + 2.81 * input : 0.331 + 0.4173 * input;
    for (int i = 0; i < 2; i++)
    {
        const float xn2 = xn * xn;
        xn = xn * (xn2 + 3 * input) / (3 * xn2 + input);
    }
    return xn;
}

static inline float process_one_sample(float input)
{
    float s = input;
    s = process_asymcompr(&input_compr, s);
    s *= gain;
    if (s < 0)
        s = -fastsqrt(fastsqrt(-s));
    else
        s = fastsqrt(fastsqrt(s));

    s = process_asymcompr(&output_compr, s);

    return s;
}

void MODFX_PROCESS(const float *main_xn, float *main_yn,
                   const float *sub_xn, float *sub_yn,
                   uint32_t frames)
{
    (void)sub_xn;
    (void)sub_yn;

    const float *__restrict x = (const float *)main_xn;
    float *__restrict y = (float *)main_yn;
    
    int this_frame_silent = next_frame_silent;
    next_frame_silent = 1;
    for (uint32_t i = 0; i < frames; i++)
    {
        for (int ch = 0; ch < 2; ch++)
        {
            float output = process_one_sample(*(x++));
            if (fabs(output) > noise_gate_lev)
            {
                next_frame_silent = 0;
            }
            if (this_frame_silent)
                output = 0;
            *(y++) = output;
        }
    }
}

void MODFX_PARAM(uint8_t index, int32_t value)
{
    const float v = q31_to_f32(value);
    if (index == k_user_modfx_param_time)
    {
        gain = 0.1 + 3.9 * v;
    }
    else if (index == k_user_modfx_param_depth)
    {
        noise_gate_lev = v;
    }
}
