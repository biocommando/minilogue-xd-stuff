#include "userrevfx.h"

#define NDELS 8
#define NPARALLEL 2
#define DEL_BUF_SZ 16000

static __sdram float delay_buf[DEL_BUF_SZ * NDELS * 2];

struct allpass_state
{
    float *mem;
    float gain;
    int mem_idx, mem_len;
};

float calculate_allpass_state(float input, struct allpass_state *s)
{
    float a = -s->gain * input;
    float b = s->mem[s->mem_idx];
    float out = a + b;
    s->mem[s->mem_idx] = input + s->gain * out;
    if (++s->mem_idx >= s->mem_len)
        s->mem_idx = 0;
    return out;
}

static const int mem_lens[NDELS] = {
    1775, 640, 120, 985, 1188, 738, 1724, 1156
};

static struct allpass_state as[2][NDELS];
static float dry_mix, wet_mix;

void REVFX_INIT(uint32_t platform, uint32_t api)
{
    (void) platform;
    (void) api;
    unsigned offset = 0;
    for (int i = 0; i < NDELS; i++)
    {
        for (int c = 0; c < 2; c++)
        {
            as[c][i].mem = &delay_buf[offset];
            offset += DEL_BUF_SZ;
        }
    }
}

void REVFX_PROCESS(float *x, uint32_t frames)
{
    for (uint32_t i = 0; i < frames; i++)
    {
        for (int c = 0; c < 2; c++)
        {
            float input = x[i * 2 + c];
            float rv_output = 0;
            for (size_t j = 0; j < NPARALLEL; j++)
            {
                float x = calculate_allpass_state(input, &as[c][j]);
                rv_output += x;
            }

            for (size_t j = NPARALLEL; j < NDELS; j++)
            {
                rv_output = calculate_allpass_state(rv_output, &as[c][j]);
            }
            x[i * 2 + c] = input * dry_mix + rv_output * wet_mix;
        }
    }
}


void REVFX_PARAM(uint8_t index, int32_t value)
{
    const float v = q31_to_f32(value);
    if (index == k_user_revfx_param_shift_depth)
    {
        dry_mix = 1 - v;
        wet_mix = v;
        float corr = 1.0f / fasterpowf(dry_mix * dry_mix + wet_mix * wet_mix, 0.5f);
        dry_mix *= corr;
        wet_mix *= corr;
    }
    else if (index == k_user_revfx_param_depth)
    {
        float gain = 0.6f + 0.38f * v;
        for (int i = 0; i < NDELS; i++)
        {
            for (int c = 0; c < 2; c++)
            {
                as[c][i].gain = gain;
            }
        }
    }
    else if (index == k_user_revfx_param_time)
    {
        float len_factor = 1 + v * 7;
        for (int i = 0; i < NDELS; i++)
        {
            for (int c = 0; c < 2; c++)
            {
                as[c][i].mem_len = mem_lens[i] + c * 31;
                if (i >= NPARALLEL)
                    as[c][i].mem_len *= len_factor;
            }
        }
    }
}
