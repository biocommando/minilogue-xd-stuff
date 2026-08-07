#include "../src/basic_oscillator.h"
#include "../src/synth_random.h"

#include "userosc.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
static BasicOscillator osc;
static float osc_mix, noise_mix;
static enum OscType waveform;
static int note_is_on;

void OSC_INIT(uint32_t platform, uint32_t api)
{
    (void)platform;
    (void)api;
    init_BasicOscillator(&osc, 48000);
    note_is_on = 0;
}

static inline void update_inc(const user_osc_param_t *const params)
{
    const float inc = osc_w0f_for_note((params->pitch) >> 8, params->pitch & 0xFF);

    BasicOscillator_setFrequency(&osc, inc);
}

void OSC_CYCLE(const user_osc_param_t *const params,
               int32_t *yn,
               const uint32_t frames)
{

    update_inc(params);

    const float shape_lfo = q31_to_f32(params->shape_lfo);

    q31_t *__restrict y = (q31_t *)yn;
    const q31_t *y_e = y + frames;

    for (; y != y_e;)
    {
        float out = 0;
        BasicOscillator_calculateNext(&osc);
        out += BasicOscillator_getValue(&osc, waveform) * osc_mix;
        out += (float)(synth_random() % 100000) * 0.00001f * noise_mix;
        out *= 1 - shape_lfo;
        if (!note_is_on)
            out = 0;

        *(y++) = f32_to_q31(out);
    }
}

void OSC_NOTEON(const user_osc_param_t *const params)
{
    BasicOscillator_randomizePhase(&osc, 1);
    update_inc(params);
    note_is_on = 1;
}

void OSC_NOTEOFF(const user_osc_param_t *const params)
{
    (void)params;
    note_is_on = 0;
}

void OSC_PARAM(uint16_t index, uint16_t value)
{
    switch (index)
    {
    case k_user_osc_param_id1:
        if (value == 0)
            waveform = OSC_TRIANGLE;
        if (value == 1)
            waveform = OSC_SINE;
        if (value == 2)
            waveform = OSC_SAW;
        if (value == 3)
            waveform = OSC_SQUARE;
        break;
    case k_user_osc_param_shape:
        osc_mix = param_val_to_f32(value);
        break;
    case k_user_osc_param_shiftshape:
        noise_mix = param_val_to_f32(value);
        break;
    default:
        break;
    }
}