#include "usermodfx.h"
#include "compclip.h"

#define N_SAMPLE_PAIRS 3000

static uint16_t b_idx = 0;
static __sdram uint8_t buf[N_SAMPLE_PAIRS * 3];
static uint8_t pair_idx = 0;
static float b_idx_inc = 0, b_idx_acc = 0;
static float dl_val[2] = {0,0}, feed = 0;

static inline int16_t to_signed_12bitval(int16_t v)
{
    return ((int16_t)(v << 4)) >> 4;
}

static inline void get2_from_12bit_buf(int16_t *s, uint32_t pos, const uint8_t *buf)
{
    const uint32_t buf_idx = pos * 3;
    const uint32_t v32bit = (*(uint32_t*)(buf + buf_idx)) & 0x00ffffff;
    *s = ((v32bit >> 12) & 0xfff);
    *(s + 1) = (v32bit & 0xfff);
}

static inline void set2_to_12bit_buf(const int16_t *s, uint32_t pos, uint8_t *buf)
{
    const uint32_t buf_idx = pos * 3;
    uint32_t *v32bit = (uint32_t*)&buf[buf_idx];
    uint32_t v = *(s + 1) & 0xfff;
    v |= ((*s & 0xfff) << 12);
    *v32bit &= 0xff000000;
    *v32bit |= v;
}

void MODFX_INIT(uint32_t platform, uint32_t api)
{
    (void) platform;
    (void) api;
    memset(buf, 0, sizeof(buf));
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
        b_idx_acc += b_idx_inc;
        if (b_idx_acc >= 1)
        {
            b_idx_acc -= 1;
            if (++pair_idx == 2)
            {
                b_idx++;
                if (b_idx >= N_SAMPLE_PAIRS)
                    b_idx = 0;
                pair_idx = 0;
            }
            int16_t d[2];
            get2_from_12bit_buf(d, b_idx, buf);
            
            const float f = (to_signed_12bitval(d[pair_idx]) / (float)0x7ff) * feed;
            
            dl_val[0] = dl_val[1];
            dl_val[1] = f;
            float new_dl_val = dl_val[0] + input;
            if (new_dl_val > 1) new_dl_val = 1;
            else if (new_dl_val < -1) new_dl_val = -1;

            d[pair_idx] = 0x7ff * new_dl_val;
            set2_to_12bit_buf(d, b_idx, buf);
        }
        const float dl_val_int = dl_val[0] + (dl_val[1] - dl_val[0]) * b_idx_acc;
        const float out = input + dl_val_int;
        *(y++) = out;
        *(y++) = out;
        x += 2;
    }
}

void MODFX_PARAM(uint8_t index, int32_t value)
{
    const float v = q31_to_f32(value);
    if (index == k_user_modfx_param_time)
    {
        b_idx_inc = (1 - v) * 0.91 + 0.09;
    }
    else if (index == k_user_modfx_param_depth)
    {
        feed = v;
    }
}
