#include <math.h>
#include "manifest_params.h"
#include "dataoscsrc.h"
#include "userosc.h"
#include "flt.h"
#include "synth_random.h"
#include "stuff_util.h"

#define MIDDLE_C_FREQ_HZ 261.6256
#define DATA_SAMPLERATE 16000

#define N_OSC 4

static float detune_factors[N_OSC], init_ph_rand;

struct data_osc
{
    struct waveform_data wfd;
    // Sample player index
    float phase;
    // Index increment
    float inc;
    // Position where to jump after reaching end
    float loopback_idx;
    float mix;
};

static struct data_osc osc[N_OSC], piano_osc;
static float mix;
static float piano_octave;
static uint8_t duck_piano;

static uint16_t phase_jump_counter, phase_jump_counter_max;

struct tape_params
{
    float pitch_jerk_threshold, pitch_jerk_amount;
    float hiss;
    float pre_gain, post_gain;
    struct filter_state lpf;
    float lpf_cutoff_max, lpf_cutoff_mod_max;
};
struct tape_params tape;

// [Detune table copied from oscstack]
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

static inline void update_detune_factors(uint8_t tbl_idx)
{
    for (int i = 0; i < N_OSC; )
    {
        detune_factors[i++] = detune_tbl[tbl_idx];
        detune_factors[i++] = 1.0f / detune_tbl[tbl_idx];
    }
    init_ph_rand = tbl_idx ? 1 : 0;
}

static inline void update_inc(const user_osc_param_t *const params)
{
     const float base_inc = osc_w0f_for_note((params->pitch) >> 8, params->pitch & 0xFF) / MIDDLE_C_FREQ_HZ * DATA_SAMPLERATE;
     for (int i = 0; i < N_OSC; i++)
     {
        osc[i].inc = base_inc * detune_factors[i];
     }
     piano_osc.inc = base_inc * piano_octave;
}

static inline float data_osc_process(struct data_osc *osc)
{
    uint32_t i = osc->phase;
    if (i >= osc->wfd.length)
        return 0;

    const float next = i + 1 < osc->wfd.length ? osc->wfd.data[i + 1] : 0;
    const float curr = osc->wfd.data[i];
    const float out = curr + (next - curr) * (osc->phase  - i);

    osc->phase += osc->inc;
    if (osc->phase >= osc->wfd.length)
        osc->phase = osc->loopback_idx;

    return out * osc->mix / 127.0f;
}

static inline void calc_mix_factors(float mod)
{
    float total = mix + mod;
    if (total < 0)
        total = 0;
    if (total > 1)
        total = 1;
    float common = 1 - piano_osc.mix;
    osc[0].mix = osc[1].mix = total * common;
    osc[2].mix = osc[3].mix = (1 - total) * common;
}

static inline void set_tape_pitch_jerks()
{
    for (int i = 0; i < N_OSC; i++)
    {
        if (synth_randomf() > tape.pitch_jerk_threshold)
        {
            osc[i].inc *= 1 - synth_randomf() * tape.pitch_jerk_amount;
            if (i == 0)
                piano_osc.inc *= 1 - synth_randomf() * tape.pitch_jerk_amount;
        }
    }
}

void OSC_INIT(uint32_t platform, uint32_t api)
{
    (void) platform;
    (void) api;

    for (int i = 0; i < N_OSC / 2; i++)
    {
        osc[i].wfd = get_choir_waveform();
        osc[i].loopback_idx = 5138;
        osc[N_OSC / 2 + i].wfd = get_string_waveform();
        osc[N_OSC / 2 + i].loopback_idx = 80;
    }
    init_filter(&tape.lpf, k_samplerate / 2, k_samplerate);
}

void OSC_CYCLE(const user_osc_param_t *const params, int32_t *yn, const uint32_t frames)
{
    update_inc(params);
    calc_mix_factors(q31_to_f32(params->shape_lfo));
    set_tape_pitch_jerks();

    float max_ampl = 0;
    OSC_LOOP(y, yn, frames)
    {
        float out = 0;
        for (int i = 0; i < N_OSC; i++)
        {
            out += data_osc_process(osc + i);
        }
        if (duck_piano)
        {
            out *= piano_osc.phase / piano_osc.wfd.length;
        }
        out += data_osc_process(&piano_osc) * 2;
        // Tape coloration
        out += synth_random_noise() * tape.hiss;
        out *= tape.pre_gain;
        if (out > 1)
            out = 1;
        else if (out < -1)
            out = -1;
        else
            out = out - out * out * out * 0.33333f;
        out = process_filter(&tape.lpf, out);
        out *= tape.post_gain;
        const float abs_out = fabsf(out);
        if (abs_out > max_ampl)
            max_ampl = abs_out;

        *(y++) = safe_f32_to_q31(out);
    }
    const float temp = tape.lpf.state0;
    float freq = tape.lpf_cutoff_max - tape.lpf_cutoff_mod_max * max_ampl;
    if (freq <= 100)
        freq = 100;
    init_filter(&tape.lpf, freq, k_samplerate);
    tape.lpf.state0 = temp;
    
    if (phase_jump_counter_max > 0)
    {
        phase_jump_counter += frames;
        if (phase_jump_counter >= phase_jump_counter_max)
        {
            for (int i = 0; i < N_OSC; i++)
            {
                osc[i].phase = osc[i].wfd.length * synth_randomf();
            }
            phase_jump_counter = 0;
        }
    }
}

void OSC_NOTEON(const user_osc_param_t *const params)
{
    (void) params;
    for (int i = 0; i < N_OSC; i++)
    {
        osc[i].phase = init_ph_rand * synth_randomf() * 32;
    }
    piano_osc.phase = 0;
}

void OSC_NOTEOFF(const user_osc_param_t *const params)
{
    (void) params;
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

void OSC_PARAM(uint16_t index, uint16_t value)
{
    switch (index)
    {
        case USER_PARAM__Detune__idx:
            update_detune_factors(value);
            break;
        case USER_PARAM__Tape_effect__idx:
            {
                const float v = value / 100.0f;
                tape.pre_gain = tape.post_gain = 1;
                tape.pre_gain = 0.5f + 15 * v * v;
                tape.post_gain = 1.0f / (1 + 4 * v);
                tape.hiss = 0.1f * v;
                tape.pitch_jerk_threshold = 1 - v * 0.2f;
                tape.pitch_jerk_amount = v * 0.1;
                tape.lpf_cutoff_max = 15000 - 8000 * v;
                tape.lpf_cutoff_mod_max = 4000 + get_log_scale_val((float)8000 / (1 << N_OCTAVES), v * 1.05);
            }
            break;
        case USER_PARAM__Piano_octave__idx:
            piano_octave = (1 << value) * 0.5f;
            break;
        case USER_PARAM__Duck_piano__idx:
            duck_piano = value;
            break;
        case USER_PARAM__Piano_waveform__idx:
            piano_osc.wfd = value ?
                get_clockenspiel_waveform() : get_piano_waveform();
            piano_osc.loopback_idx = piano_osc.wfd.length;
            break;
        case USER_PARAM__Granule_size__idx:
            phase_jump_counter_max = 480 * value;
            break;
        case k_user_osc_param_shape:
            mix = param_val_to_f32(value);
            break;
        case k_user_osc_param_shiftshape:
            piano_osc.mix = param_val_to_f32(value);
            break;
        default:
            break;
    }
}
