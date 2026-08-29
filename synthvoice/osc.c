#include "adsr_envelope.h"
#include "basic_oscillator.h"
#include "moog_filter.h"
#include "userosc.h"
#include "synth_random.h"
#include "stuff_util.h"

static AdsrEnvelope env;
static MicrotrackerMoog filter;
static BasicOscillator osc, sub;

static enum OscType waveform = OSC_TRIANGLE, wf_sqr = OSC_SQUARE;
static enum OscType *sub_waveform = NULL;
static float sub_mix = 0, main_mix = 1;
static uint8_t noise_on = 0, sub_noise_mask = 7;

#define WT_LEN 128
#define WT_NUM 4
#define WT_TYPE_NOISE 0
#define WT_TYPE_NOISE_SAW 1
#define WT_TYPE_PULSE 2
#define WT_TYPE_THIN_PULSE 3

static float wavetables[WT_NUM][WT_LEN];

void OSC_INIT(uint32_t platform, uint32_t api)
{
    (void) platform;
    (void) api;
    init_AdsrEnvelope(&env);
    init_MicrotrackerMoog(&filter, k_samplerate);
    init_BasicOscillator(&osc, k_samplerate);
    init_BasicOscillator(&sub, k_samplerate);
    for (int wi = 0; wi < WT_NUM; wi++)
    {
        for (int i = 0; i < WT_LEN; i++)
        {
            float y;
            if (wi == WT_TYPE_NOISE)
                y = 2 * ((synth_random() & 0xFFFF) / (float) 0xFFFF - 0.5);
            else if (wi == WT_TYPE_NOISE_SAW)
                y = 2 * ((i / (float) WT_LEN - 0.5) + (synth_random() & 0xFFFF) / (float) 0xFFFF - 0.5);
            else if (wi == WT_TYPE_PULSE)
                y = i > 10 ? 1 : -1;
            else if (wi == WT_TYPE_THIN_PULSE)
                y = i > 5 ? 1 : -1;

            wavetables[wi][i] = y;
        }
    }
    sub_waveform = &waveform;
}

static inline void update_inc(const user_osc_param_t *const params)
{
    const float inc = osc_w0f_for_note((params->pitch) >> 8, params->pitch & 0xFF);
    BasicOscillator_setFrequency(&osc, inc);
    BasicOscillator_setFrequency(&sub, inc * 0.5);
}

void OSC_CYCLE(const user_osc_param_t *const params, int32_t *yn, const uint32_t frames)
{

    update_inc(params);

    const float shape_lfo = q31_to_f32(params->shape_lfo);
    float last_noise = 0;
    uint8_t update_sub_noise = 0;

    OSC_LOOP(y, yn, frames)
    {
        AdsrEnvelope_calculateNext(&env);
        float mod = AdsrEnvelope_getEnvelope(&env);
        mod += shape_lfo;
        MicrotrackerMoog_setModulation(&filter, mod);
        BasicOscillator_calculateNext(&osc);
        BasicOscillator_calculateNext(&sub);
        float out;
        if (noise_on)
        {
            const float n = (synth_random() & 0xFFFFF) / ((float)0x7FFFF) - 1;
            out = n * main_mix;
            if (update_sub_noise == 0)
                last_noise = n;
            out += last_noise * sub_mix;
        }
        else
        {
            out = BasicOscillator_getValue(&osc, waveform) * main_mix;
            out += BasicOscillator_getValue(&sub, *sub_waveform) * sub_mix;
        }
        out = MicrotrackerMoog_calculate(&filter, out);

        *(y++) = safe_f32_to_q31(out);

        update_sub_noise = (update_sub_noise + 1) & sub_noise_mask;
    }
}

void OSC_NOTEON(const user_osc_param_t *const params)
{
    update_inc(params);
    AdsrEnvelope_trigger(&env);
}

void OSC_NOTEOFF(const user_osc_param_t *const params)
{
    (void) params;
    AdsrEnvelope_release(&env);
}


void OSC_PARAM(uint16_t index, uint16_t value)
{
    switch (index)
    {
        case k_user_osc_param_id1:
            AdsrEnvelope_setAttack(&env, 4 * k_samplerate * (value / 100.0));
            break;
        case k_user_osc_param_id2:
            AdsrEnvelope_setDecay(&env, 4 * k_samplerate * (value / 100.0));
            break;
        case k_user_osc_param_id3:
            AdsrEnvelope_setSustain(&env, value / 100.0);
            break;
        case k_user_osc_param_id4:
            AdsrEnvelope_setRelease(&env, 4 * k_samplerate * (value / 100.0));
            break;
        case k_user_osc_param_id5:
            noise_on = 0;
            if (value == 0)
                waveform = OSC_SAW;
            else if (value == 1)
                waveform = OSC_SQUARE;
            else if (value == 2)
                waveform = OSC_TRIANGLE;
            else if (value == 3)
                waveform = OSC_SINE;
            else if (value == 11)
                noise_on = 1;
            else if (value >= 4)
            {
                waveform = OSC_WT;
                int wt_index = WT_TYPE_NOISE;
                float win = 1;
                if (value == 4)
                    wt_index = WT_TYPE_PULSE;
                else if (value == 5)
                    wt_index = WT_TYPE_THIN_PULSE;
                else if (value == 6)
                    wt_index = WT_TYPE_NOISE_SAW;
                else
                {
                    win = (value - 6) * 0.25;
                    if (win > 1)
                        win = 1;
                }

                float *wt = wavetables[wt_index];
                BasicOscillator_setWavetable(&osc, wt, WT_LEN);
                BasicOscillator_setWaveTableParams(&osc, 0, win);
                BasicOscillator_setWavetable(&sub, wt, WT_LEN);
                BasicOscillator_setWaveTableParams(&sub, 0, win);
            }
            break;
        case k_user_osc_param_id6:
            if (value > 100)
            {
                sub_waveform = &wf_sqr;
                value -= 100;
                sub_noise_mask = 0xf;
            }
            else
            {
                sub_waveform = &waveform;
                value = 100 - value;
                sub_noise_mask = 7;
            }
            sub_mix = value / 100.0;
            main_mix = 1 - sub_mix;
            break;
        case k_user_osc_param_shape:
            MicrotrackerMoog_setCutoff(&filter, param_val_to_f32(value));
            break;
        case k_user_osc_param_shiftshape:
            MicrotrackerMoog_setResonance(&filter, param_val_to_f32(value));
            break;
        default:
            break;
    }
}
