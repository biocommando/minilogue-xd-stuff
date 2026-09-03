#include "synth_random.h"
#include "basic_oscillator.h"
#include "userosc.h"
#include "manifest_params.h"
#include "waveform.h"
#include "stuff_util.h"

#define N_OSC 3
static BasicOscillator osc[N_OSC];

static enum OscType waveform[N_OSC] = { OSC_TRIANGLE, OSC_TRIANGLE, OSC_TRIANGLE, };
static float osc_mix[N_OSC] = { 0, 0, 0, };

static float noise_mix = 0;
static float wt_pos = 0;
static float inclist[N_OSC] = { 1, 1, 1, };

void OSC_INIT(uint32_t platform, uint32_t api)
{
    (void) platform;
    (void) api;
    for (int i = 0; i < N_OSC; i++)
    {
        init_BasicOscillator(&osc[i], k_samplerate);
        BasicOscillator_setWavetable(&osc[i], (float *) get_wt(), get_wt_size());
    }
}

static inline void update_inc(const user_osc_param_t *const params)
{
    const float inc = osc_w0f_for_note((params->pitch) >> 8, params->pitch & 0xFF);
    for (int i = 0; i < N_OSC; i++)
        BasicOscillator_setFrequency(&osc[i], inc * inclist[i]);
}

void OSC_CYCLE(const user_osc_param_t *const params, int32_t *yn, const uint32_t frames)
{

    update_inc(params);

    const float shape_lfo = q31_to_f32(params->shape_lfo);
    float mod_wt_pos = wt_pos + shape_lfo;
    while (mod_wt_pos > 1)
        mod_wt_pos -= 1;
    while (mod_wt_pos < 0)
        mod_wt_pos += 1;
    for (int i = 0; i < N_OSC; i++)
        BasicOscillator_setWaveTableParams(&osc[i], mod_wt_pos, 0.25);

    OSC_LOOP(y, yn, frames)
    {
        float out = 0, amp_mult = 1 - shape_lfo;
        for (int i = 0; i < N_OSC; i++)
        {
            BasicOscillator_calculateNext(&osc[i]);
            float o = BasicOscillator_getValue(&osc[i], waveform[i]) * osc_mix[i];
            if (waveform[i] != OSC_WT)
                o *= amp_mult;
            out += o;
        }
        out += synth_random_noise() * noise_mix;

        *(y++) = safe_f32_to_q31(out);
    }
}

static uint8_t first_note_on = 2;
void OSC_NOTEON(const user_osc_param_t *const params)
{
    if (first_note_on)
    {
        // synth_random's distribution isn't the best in the world so we're
        // trying very hard to get a decent initial guess.
        // On first two notes we separately reset the random engine with
        // better entropy guesses.
        first_note_on--;
        synth_random_reset(params->pitch ^ (f32_to_q31(osc[0].frequency)));
    }
    for (int i = 0; i < N_OSC; i++)
        BasicOscillator_randomizePhase(&osc[i], 1);
    update_inc(params);
}

void OSC_NOTEOFF(const user_osc_param_t *const params)
{
    (void) params;
}

// Generated using:
// print(','.join([ ('' if i%10>0 else '\n') + f'{x:.8}' for i,x in enumerate([ 2**(i/100/12/3) for i in range(101) ])]))
// This gets max 100/300 = 33.3 semitones which means 66.7 semis spread
static const float detune_tbl[] = {
    1.0, 1.0001926, 1.0003852, 1.0005778, 1.0007705, 1.0009632, 1.0011559, 1.0013487, 1.0015415, 1.0017344,
    1.0019273, 1.0021202, 1.0023132, 1.0025062, 1.0026992, 1.0028923, 1.0030854, 1.0032786, 1.0034717, 1.003665,
    1.0038582, 1.0040515, 1.0042449, 1.0044383, 1.0046317, 1.0048251, 1.0050186, 1.0052121, 1.0054057, 1.0055993,
    1.0057929, 1.0059866, 1.0061803, 1.0063741, 1.0065679, 1.0067617, 1.0069556, 1.0071494, 1.0073434, 1.0075374,
    1.0077314, 1.0079254, 1.0081195, 1.0083136, 1.0085078, 1.008702, 1.0088962, 1.0090905, 1.0092848, 1.0094791,
    1.0096735, 1.009868, 1.0100624, 1.0102569, 1.0104514, 1.010646, 1.0108406, 1.0110353, 1.01123, 1.0114247,
    1.0116194, 1.0118142, 1.0120091, 1.0122039, 1.0123989, 1.0125938, 1.0127888, 1.0129838, 1.0131789, 1.013374,
    1.0135691, 1.0137643, 1.0139595, 1.0141547, 1.01435, 1.0145453, 1.0147407, 1.0149361, 1.0151315, 1.015327,
    1.0155225, 1.0157181, 1.0159136, 1.0161093, 1.0163049, 1.0165006, 1.0166964, 1.0168921, 1.017088, 1.0172838,
    1.0174797, 1.0176756, 1.0178716, 1.0180676, 1.0182636, 1.0184597, 1.0186558, 1.018852, 1.0190482, 1.0192444,
    1.0194406
};

static int o2_octave = 0, o2_detune = 0;
void set_o2_base_inc()
{
    const int oct_mult = 1 << o2_octave;
    float inc = 0.25 * oct_mult;
    inclist[1] = inc * detune_tbl[o2_detune];
    inclist[2] = inc / detune_tbl[o2_detune];
    if (o2_detune == 0)
        osc_mix[2] = 0;
    else
        osc_mix[2] = osc_mix[1];
}

void set_waveform(int wi, uint16_t value)
{
    if (value == 0)
        waveform[wi] = OSC_SAW;
    else if (value == 1)
        waveform[wi] = OSC_SQUARE;
    else if (value == 2)
        waveform[wi] = OSC_TRIANGLE;
    else if (value == 3)
        waveform[wi] = OSC_SINE;
    else if (value == 4)
        waveform[wi] = OSC_WT;
}

void OSC_PARAM(uint16_t index, uint16_t value)
{
    switch (index)
    {
        case USER_PARAM__O1_Waveform__idx:
            set_waveform(0, value);
            break;
        case USER_PARAM__O1_Atten__idx:
            osc_mix[0] = 1 - value / 100.0;
            break;
        case USER_PARAM__O2_Waveform__idx:
            set_waveform(1, value);
            set_waveform(2, value);
            break;
        case USER_PARAM__O2_Mix__idx:
            osc_mix[1] = value / 100.0;
            set_o2_base_inc();
            break;
        case USER_PARAM__O2_Octave__idx:
            o2_octave = value;
            set_o2_base_inc();
            break;
        case USER_PARAM__O2_Detune__idx:
            o2_detune = value;
            set_o2_base_inc();
            break;
        case k_user_osc_param_shape:
            wt_pos = param_val_to_f32(value);
            break;
        case k_user_osc_param_shiftshape:
            noise_mix = param_val_to_f32(value);
            break;
        default:
            break;
    }
}
