#include "usermodfx.h"
#include "float_math.h"

#define FLANGER_BUF_LEN 1000
#define DELAY_BUF_LEN 5000

static uint16_t delay, flanger_pos, delay_pos;

static __sdram float flanger_buf[DELAY_BUF_LEN];
static __sdram float delay_buf[DELAY_BUF_LEN];

static float flanger_mix;
static float osc_phase[3], osc_inc[3], osc_val[3];
static uint8_t osc_hold[3];

void MODFX_INIT(uint32_t platform, uint32_t api)
{
    (void) platform;
    (void) api;
    osc_inc[0] = 0.45f / 48000;
    osc_inc[1] = 1.45f / 48000;
    osc_inc[2] = 2.45f / 48000;
}

static inline float process_osc(int i)
{
    if (osc_hold[i] == 0)
    {
        osc_phase[i] += osc_inc[i] * 4;
        osc_phase[i] = osc_phase[i] - (int)osc_phase[i];
        osc_val[i] = fastsinfullf(osc_phase[i] * 3.14159265358979323846f * 2);
        osc_hold[i] = 4;
    }
    osc_hold[i]--;
    return osc_val[i];
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
        flanger_buf[flanger_pos] = input;
        if (++flanger_pos == FLANGER_BUF_LEN)
            flanger_pos = 0;
        float flanger_out = 0;
        for (int j = 0; j < 3; j++)
        {
            int fpos = (int)flanger_pos - 600 + process_osc(j) * 120;
            if (fpos < 0)
                fpos += FLANGER_BUF_LEN;
            else if (fpos >= FLANGER_BUF_LEN)
                fpos -= FLANGER_BUF_LEN;
            flanger_out -= flanger_buf[fpos];
        }
        
        input = flanger_out * flanger_mix * 0.5f + input * (1 - flanger_mix);

        *(y++) = input;
        int delay_read_pos = delay_pos - delay;
        if (delay_read_pos < 0)
            delay_read_pos += DELAY_BUF_LEN;
        *(y++) = delay ? delay_buf[delay_read_pos] : input;
        delay_buf[delay_pos] = input;
        if (++delay_pos >= DELAY_BUF_LEN)
            delay_pos = 0;
        x += 2;
    }
}

void MODFX_PARAM(uint8_t index, int32_t value)
{
    const float v = q31_to_f32(value);
    if (index == k_user_modfx_param_time)
    {
        delay = v * DELAY_BUF_LEN;
    }
    else if (index == k_user_modfx_param_depth)
    {
        flanger_mix = v;
    }
}
