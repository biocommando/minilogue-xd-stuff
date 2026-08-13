#include "2opfmsynth.h"
#include "userosc.h"
#include "stuff_util.h"

static VoiceFmSynth2Op v1, v2;

static float mix = 0.5f;
static float asym_clip = 1.0f;

static inline float calc_exp_env(int16_t p)
{
    if (!p)
        return 1;
    const uint16_t ap = p > 0 ? p : -p;
    const float len_s = ap / 16.667f;   //Max 6 sec.
    const float target = 0.01;  //=-40 dB
    float env = powf(target, 1.0f / (k_samplerate * len_s));
    if (p < 0 && env != 0)
    {
        env = 1 / env;
    }
    return env;
}

void OSC_INIT(uint32_t platform, uint32_t api)
{
    (void) platform;
    (void) api;
    memset(&v1, 0, sizeof(v1));
    memset(&v2, 0, sizeof(v2));
}

static inline void update_inc(const user_osc_param_t *const params)
{
    const float inc = osc_w0f_for_note((params->pitch) >> 8, params->pitch & 0xFF);
    set_note_inc(&v1, inc);
    set_note_inc(&v2, inc);
}

void OSC_CYCLE(const user_osc_param_t *const params, int32_t *yn, const uint32_t frames)
{

    update_inc(params);

    const float shape_lfo = q31_to_f32(params->shape_lfo);
    float modulated_mix = shape_lfo + mix;
    if (modulated_mix > 1)
        modulated_mix = 1;
    else if (modulated_mix < 0)
        modulated_mix = 0;
    const float mix2 = modulated_mix;
    const float mix1 = 1 - modulated_mix;

    OSC_LOOP(y, yn, frames)
    {
        const float o1 = process_voice_fm_synth_2op(&v1, asym_clip) * mix1;
        const float o2 = process_voice_fm_synth_2op(&v2, asym_clip) * mix2;
        const float out = o1 + o2;

        *(y++) = safe_f32_to_q31(out);
    }
}

void OSC_NOTEON(const user_osc_param_t *const params)
{
    update_inc(params);
    reset_voice(&v1);
    reset_voice(&v2);
}

void OSC_NOTEOFF(const user_osc_param_t *const params)
{
    (void) params;
}

#define SET_FREQ_RATIO_PARAM(v) {v.freq_ratio = (value + 1) / 8.0f;}
#define SET_MOD_AMT_PARAM(v) {v.original_modulation_amount = value == 0 ? default_mod_amount : value / 10.0f;}
#define SET_MOD_ENV_PARAM(v) {v.original_modulation_env = calc_exp_env((int16_t)value - 100);}

void OSC_PARAM(uint16_t index, uint16_t value)
{
    const float default_mod_amount = 0.5;
    switch (index)
    {
        case k_user_osc_param_id1:
            SET_FREQ_RATIO_PARAM(v1);
            break;
        case k_user_osc_param_id2:
            SET_MOD_AMT_PARAM(v1);
            break;
        case k_user_osc_param_id3:
            SET_MOD_ENV_PARAM(v1);
            break;
        case k_user_osc_param_id4:
            SET_FREQ_RATIO_PARAM(v2);
            break;
        case k_user_osc_param_id5:
            SET_MOD_AMT_PARAM(v2);
            break;
        case k_user_osc_param_id6:
            SET_MOD_ENV_PARAM(v2);
            break;
        case k_user_osc_param_shape:
            mix = param_val_to_f32(value);
            break;
        case k_user_osc_param_shiftshape:
            asym_clip = 1.0f - param_val_to_f32(value);
            break;
        default:
            break;
    }
}
