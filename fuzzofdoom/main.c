#include "usermodfx.h"
#include "flt.h"

#define SAMPLERATE 48000
#define OVERSAMPLING 4

static float gain = 1;
static float blend = 0.5;
static float blend_inv = 0.5;
static struct filter_state tone_filter, downsampling_filters[2];

void MODFX_INIT(uint32_t platform, uint32_t api)
{
    (void) platform;
    (void) api;
    const float cut_param = 0.5 / 2;
    const float cutoff = (cut_param * cut_param) * 0.5 * SAMPLERATE;
    init_filter(&tone_filter, cutoff, SAMPLERATE * OVERSAMPLING);
    for (int i = 0; i < 2; i++)
    {
        init_filter(&downsampling_filters[i], 18000, SAMPLERATE * OVERSAMPLING);
    }
}

static inline float fuzz(const float input)
{
    const float gate = 0.05;
    float i = input * gain;
    float gated = i > gate ? 1 : -1;
    return i * blend_inv + gated * blend;
}

static inline float process_one_sample(float input)
{
    float out = fuzz(input);
    out = out > 1 ? 1 : out;
    out = out < -1 ? -1 : out;
    out = process_filter(&tone_filter, out);
    return out;
}

static inline float fuzz0()
{
    const float gated = -1;
    return gated * blend;
}

static inline float process_one_sample0()
{
    const float out = fuzz0();
    return process_filter(&tone_filter, out);
}

void MODFX_PROCESS(const float *main_xn, float *main_yn, const float *sub_xn, float *sub_yn, uint32_t frames)
{
    (void) sub_xn;
    (void) sub_yn;

    const float *__restrict x = (const float *) main_xn;
    float *__restrict y = (float *) main_yn;

    for (uint32_t i = 0; i < frames; i++)
    {
        float input = *x, output;
        output = process_one_sample(input);
        output = process_filter(&downsampling_filters[0], output);
        output = process_filter(&downsampling_filters[1], output);
        for (int o = 0; o < OVERSAMPLING - 1; o++)
        {
            output = process_one_sample0();
            output = process_filter(&downsampling_filters[0], output);
            output = process_filter(&downsampling_filters[1], output);
        }
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
        gain = 1 + 9 * v * v;
    }
    else if (index == k_user_modfx_param_depth)
    {
        blend = v;
        blend_inv = 1 - blend;
    }
}
