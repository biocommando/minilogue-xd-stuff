#include "usermodfx.h"
#include "synth_random.h"
#include "flt.h"

static const float target_speed = 1.0f;
static float current_speed = 1.0f;
static float integrator = 0.0f;
static float disturbance_gain = 1;
static uint8_t disturbance_prob = 5;

static float Kp = 0.05f;
static float Ki = 0.001f;

static inline float update_tape_speed(uint32_t frames) {
    float disturbance = 0.0f;
    if ((synth_random() & 0x3FF) < disturbance_prob * frames) {
        disturbance = (-0.01f - (synth_random()&0xFFFFF)/(float)0xFFFFF * 0.05f) * disturbance_gain;
    }
    
    current_speed += disturbance;

    float error = target_speed - current_speed;
    integrator += error;

    if (integrator > 1.0f) integrator = 1.0f;
    if (integrator < -1.0f) integrator = -1.0f;

    float control_signal = (Kp * error) + (Ki * integrator);

    current_speed += control_signal;

    return current_speed;
}

#define DL_LEN 128
static float delay_line[DL_LEN], dl_acc = 0, dl_val[2] = {0, 0};
static uint16_t dl_idx = 0;
static struct filter_state lpf;
static float gain = 1.5, out_gain = 1;
static float lpf_cutoff_mult = 1;
static float hiss_gain = 1;
static float pitch_noise_gain = 1;

void MODFX_INIT(uint32_t platform, uint32_t api)
{
    (void) platform;
    (void) api;
    init_filter(&lpf, 10000, 48000);
}

void MODFX_PROCESS(const float *main_xn, float *main_yn, const float *sub_xn, float *sub_yn, uint32_t frames)
{
    (void) sub_xn;
    (void) sub_yn;

    update_tape_speed(frames);

    const float *__restrict x = (const float *) main_xn;
    float *__restrict y = (float *) main_yn;

    float max_ampl = 0;
    static int cc = 0;
    for (uint32_t i = 0; i < frames; i++)
    {
        float input = *x;
        if (i & 1)
        {
            float low_freq_noise = ((synth_random()&0xFFFFF)/(float)0xFFFFF - 0.5f) * 0.01f;
            current_speed += low_freq_noise * pitch_noise_gain;
        }
        dl_acc += current_speed;
        if (dl_acc >= 1)
        {
            dl_acc -= 1;
            if (dl_acc >= 1) dl_acc -= 1;
            dl_val[0] = delay_line[dl_idx];
            delay_line[dl_idx] = input;
            if (++dl_idx == DL_LEN)
                dl_idx = 0;
            dl_val[1] = delay_line[dl_idx];
        }
        float output = dl_val[0] + (dl_val[1] - dl_val[0]) * dl_acc;
        output *= gain;
        if (output > max_ampl) max_ampl = input;
        else if (output < -max_ampl) max_ampl = -input;
        output = output - output * output * output / 3;
        output = process_filter(&lpf, output) + (synth_random()&0xFFFFF)/(float)0x7FFFFFF * hiss_gain;
        output *= out_gain;
        *(y++) = output;
        *(y++) = output;
        x += 2;
    }
    const float temp = lpf.state0;
    init_filter(&lpf, 12000 * lpf_cutoff_mult - 10000 * lpf_cutoff_mult * max_ampl, 48000);
    lpf.state0 = temp;
}

void MODFX_PARAM(uint8_t index, int32_t value)
{
    const float v = q31_to_f32(value);
    if (index == k_user_modfx_param_time)
    {
        disturbance_gain = 0.2 + v * 1.5;
        disturbance_prob = (uint8_t)(1.5 + 4 * v);
        pitch_noise_gain = v;
    }
    else if (index == k_user_modfx_param_depth)
    {
        gain = 1 + v * 6;
        out_gain = 1 / gain;
        lpf_cutoff_mult = 1 - v * 0.4;
        hiss_gain = 0.7 + v * 4;
    }
}
