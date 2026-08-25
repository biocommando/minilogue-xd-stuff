#include "usermodfx.h"
#include "constants.h"
#include "simple_oscillator.h"
#include "synth_random.h"
#include "flt.h"

static SimpleOscillator wow_osc;
static float wow_gain;

static float delay_line[DL_LEN], dl_acc = 0, dl_val[2] = { 0, 0 };

static uint16_t dl_idx = 0;
static struct filter_state lpf;
static float gain = 1.5, out_gain = 1;
static float lpf_cutoff_max, lpf_cutoff_mod_max;
static float hiss_gain = 1;
static float pitch_noise_gain = 1;
static float hum_gain = 1;
static uint16_t hum_counter = 0;
static uint32_t noise_gate_counter = 0;

#define SAMPLERATE 48000
#define HUM_COUNTER_RESET (48000 / HUM_FREQ_HZ)
#define HUM_COUNTER_HALF_WAVE (HUM_COUNTER_RESET / 2)
#define INPUT_NOISE_GATE_THD 1e-4

static const float target_speed = 1.0f;
static float current_speed = 1.0f;
static float integrator = 0.0f;
static float disturbance_gain = 1;
static uint8_t disturbance_prob = 5;

static float Kp = 0.05f;
static float Ki = 0.001f;

static inline float update_tape_speed(uint32_t frames)
{
    float disturbance = 0.0f;
    if ((synth_random() & 0x3FF) < disturbance_prob * frames)
    {
        disturbance = (-0.01f - (synth_random() & 0xFFFFF) / (float) 0xFFFFF * 0.05f) * disturbance_gain;
    }
    SimpleOscillator_calculateNext(&wow_osc);
    disturbance += SimpleOscillator_getValue(&wow_osc, OSC_TRIANGLE) * wow_gain;

    current_speed += disturbance;

    float error = target_speed - current_speed;
    integrator += error;

    if (integrator > 1.0f)
        integrator = 1.0f;
    if (integrator < -1.0f)
        integrator = -1.0f;

    float control_signal = (Kp * error) + (Ki * integrator);

    current_speed += control_signal;

    return current_speed;
}

void MODFX_INIT(uint32_t platform, uint32_t api)
{
    (void) platform;
    (void) api;
    init_filter(&lpf, 10000, SAMPLERATE);
    wow_osc.frequency = WOW_FREQ_HZ / (float)SAMPLERATE;
    wow_osc.phase = wow_osc.frequency;
}

void MODFX_PROCESS(const float *main_xn, float *main_yn, const float *sub_xn, float *sub_yn, uint32_t frames)
{
    (void) sub_xn;
    (void) sub_yn;

    update_tape_speed(frames);

    const float *__restrict x = (const float *) main_xn;
    float *__restrict y = (float *) main_yn;

    float max_ampl = 0;
    for (uint32_t i = 0; i < frames; i++)
    {
        float input = *x;
        if (input < INPUT_NOISE_GATE_THD && input > -INPUT_NOISE_GATE_THD)
        {
            if (noise_gate_counter < SAMPLERATE * 5)
                noise_gate_counter++;
            else
            {
                *(y++) = *(x++);
                *(y++) = *(x++);
                continue;
            }
        }
        else
            noise_gate_counter = 0;

        input += ((synth_random() & 0xFFFFF) / (float) 0x7FFFFFF) * hiss_gain;
        hum_counter = hum_counter < HUM_COUNTER_RESET ? hum_counter + 1 : 0;
        input += hum_counter < HUM_COUNTER_HALF_WAVE ? -hum_gain : hum_gain;
        if (i & 1)
        {
            float low_freq_noise = ((synth_random() & 0xFFFFF) / (float) 0xFFFFF - 0.5f) * 0.01f;
            current_speed += low_freq_noise * pitch_noise_gain;
        }
        dl_acc += current_speed;
        if (dl_acc >= 1)
        {
            dl_acc -= 1;
            if (dl_acc >= 1)
                dl_acc -= 1;
            dl_val[0] = delay_line[dl_idx];
            delay_line[dl_idx] = input;
            if (++dl_idx == DL_LEN)
                dl_idx = 0;
            dl_val[1] = delay_line[dl_idx];
        }
        float output = dl_val[0] + (dl_val[1] - dl_val[0]) * dl_acc;
        output *= gain;

        if (output > COMPR_THD)
        {
            output = COMPR_THD + (output - COMPR_THD) * COMPR_RAT;
        }
        else if (output < -COMPR_THD)
        {
            output = -COMPR_THD + (output - -COMPR_THD) * COMPR_RAT;
        }
        if (output > 1)
            output = 1;
        else if (output < -1)
            output = -1;
        else
            output = (output - output * output * output / 3) * 1.5;

        output = process_filter(&lpf, output);
        output *= out_gain;

        if (output > max_ampl)
            max_ampl = output;
        else if (output < -max_ampl)
            max_ampl = -output;

        *(y++) = output;
        *(y++) = output;
        x += 2;
    }
    const float temp = lpf.state0;
    float freq = lpf_cutoff_max - lpf_cutoff_mod_max * max_ampl;
    if (freq <= 100)
        freq = 100;
    init_filter(&lpf, freq, SAMPLERATE);
    lpf.state0 = temp;
}

#define N_OCTAVES 16

static float get_log_scale_val(float freq, float v)
{
    int oct = v * N_OCTAVES;
    float new_freq = freq * (1 << oct);
    v -= (float) oct / N_OCTAVES;
    new_freq += freq * v * N_OCTAVES;
    return new_freq;
}

void MODFX_PARAM(uint8_t index, int32_t value)
{
    const float v = q31_to_f32(value);
    if (index == k_user_modfx_param_time)
    {
        disturbance_gain = SPEED_DISTURBANCE_MIN + v * SPEED_DISTURBANCE_DELTA;
        disturbance_prob = (uint8_t) (SPEED_DISTURBANCE_PROB_MIN + SPEED_DISTURBANCE_PROB_DELTA * v);
        pitch_noise_gain = v;
        Kp = SPEED_KP_MIN - v * SPEED_KP_DELTA;
        wow_gain = WOW_GAIN * v * v;
    }
    else if (index == k_user_modfx_param_depth)
    {
        gain = 1 + v * v * GAIN_MULT;
        out_gain = 1 / (GAIN_COMP_RANGE_MIN + GAIN_COMP_RANGE_DELTA * v);
        lpf_cutoff_max = FLT_CUT_MAX - FLT_CUT_DELTA * v;
        lpf_cutoff_mod_max = FLT_CUT_MOD_MIN + get_log_scale_val((float)FLT_CUT_MOD_DELTA / (1 << N_OCTAVES), v * 1.05);
        hiss_gain = HISS_MIN + HISS_DELTA * v;
        hum_gain = HUM_MIN + HUM_DELTA * v * v;
    }
}
