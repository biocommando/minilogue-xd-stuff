#include "synth_random.h"
#include "simple_oscillator.h"
#include "nearctic_filter.h"
#include "userosc.h"
#include "stuff_util.h"
#include "manifest_params.h"

static NearcticFilter filter;
static SimpleOscillator osc[2];

static float noise_mix, resonance, osc_mix[2];

void OSC_INIT(uint32_t platform, uint32_t api)
{
    (void) platform;
    (void) api;
    NearcticFilter_init(&filter, k_samplerate);
}
#define MIDIMAPPING_SZ 109
static const float midimapping[MIDIMAPPING_SZ] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
1e-05, 
1e-05, 
1e-05, 
1e-05, 
1e-05, 
1e-05, 
1e-05, 
1e-05, 
0.00011, 
0.00035, 
0.00061, 
0.00089, 
0.00118, 
0.00149, 
0.00182, 
0.00216, 
0.00253, 
0.00292, 
0.00334, 
0.00377, 
0.00424, 
0.00473, 
0.00526, 
0.00581, 
0.0064, 
0.00702, 
0.00769, 
0.00839, 
0.00913, 
0.00992, 
0.01076, 
0.01165, 
0.01259, 
0.01359, 
0.01465, 
0.01578, 
0.01697, 
0.01824, 
0.01959, 
0.02102, 
0.02253, 
0.02416, 
0.02586, 
0.02768, 
0.02961, 
0.03167, 
0.03387, 
0.03618, 
0.03864, 
0.04126, 
0.04405, 
0.04704, 
0.05022, 
0.0536, 
0.05719, 
0.06096, 
0.06506, 
0.0694, 
0.07402, 
0.07897, 
0.08432, 
0.08985, 
0.09595, 
0.10241, 
0.10926, 
0.1166, 
0.12442, 
0.13299, 
0.1419, 
0.15187, 
0.16218, 
0.17346, 
0.18538, 
0.19856, 
0.21249, 
0.22766, 
0.24438, 
0.2619, 
0.28193, 
0.30276, 
0.32543, 
0.35016, 
0.37795, 
0.40785, 
0.44189,
};

static inline void update_inc(const user_osc_param_t *const params)
{
    const float inc = osc_w0f_for_note((params->pitch) >> 8, params->pitch & 0xFF);
    osc[0].frequency = inc;
    osc[1].frequency = inc * 0.5;
    
    uint32_t note = (params->pitch) >> 8;
    if (note + 1 >= MIDIMAPPING_SZ)
        note = MIDIMAPPING_SZ - 2;    
    const float factor = (params->pitch & 0xFF) / (float)0xFF;
    NearcticFilter_setCutoff(&filter, midimapping[note] + (midimapping[note + 1] - midimapping[note]) * factor);
}

void OSC_CYCLE(const user_osc_param_t *const params, int32_t *yn, const uint32_t frames)
{

    update_inc(params);

    const float shape_lfo = q31_to_f32(params->shape_lfo);
    float mod_res = resonance + shape_lfo;
    if (mod_res > 1) mod_res = 1;
    if (mod_res < 0) mod_res = 0;
    NearcticFilter_setResonance(&filter, mod_res);

    OSC_LOOP(y, yn, frames)
    {
        const float n = (synth_random() & 0xFFFFF) / ((float)0x7FFFF) - 1;
        float out = n * noise_mix;
        for (int o = 0; o < 2; o++)
        {
            SimpleOscillator_calculateNext(osc + o);
            out += SimpleOscillator_getValue(osc + o, o ? OSC_SAW : OSC_SQUARE) * osc_mix[o];
        }
        out = NearcticFilter_calculate(&filter, out);

        *(y++) = safe_f32_to_q31(out);
    }
}

void OSC_NOTEON(const user_osc_param_t *const params)
{
    (void) params;
}

void OSC_NOTEOFF(const user_osc_param_t *const params)
{
    (void) params;
}


void OSC_PARAM(uint16_t index, uint16_t value)
{
    switch (index)
    {
        case USER_PARAM__Osc1_mix__idx:
            osc_mix[0] = value / 100.0;
            break;
        case USER_PARAM__Osc2_mix__idx:
            osc_mix[1] = value / 100.0;
            break;
        case k_user_osc_param_shape:
            resonance = 0.5 + 0.5 * param_val_to_f32(value);
            break;
        case k_user_osc_param_shiftshape:
            noise_mix = param_val_to_f32(value);
            break;
        default:
            break;
    }
}
