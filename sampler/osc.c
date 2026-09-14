#include "userosc.h"
#include "manifest_params.h"
#include "stuff_util.h"
#include "segments.h"
#include "simple_oscillator.h"

#define MIDDLE_C_FREQ_HZ 261.6256

static struct {
    float split, retrig_amp, flanger_mix;
    uint8_t kb_track, linint, reverse;
    uint16_t retrig;
} user_params;

static uint16_t retrig_counter;

struct waveform_data
{
    const uint8_t *data;
    uint16_t length;
};

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

static struct data_osc osc;
static float base_freq;
static uint16_t data_samplerate;
static uint8_t bits;
static float delay_buf[200];
static uint8_t delay_idx;
static SimpleOscillator flanger_osc;

#define DATA_LEN 30000
static uint8_t waveform[DATA_LEN];

static void set_sample_metadata_defaults()
{
    base_freq = MIDDLE_C_FREQ_HZ;
    data_samplerate = 16000;
    bits = 8;
}

static inline void update_inc(const user_osc_param_t *const params)
{
    if (user_params.kb_track)
        osc.inc = osc_w0f_for_note((params->pitch) >> 8, params->pitch & 0xFF) / base_freq * data_samplerate;
    else
        osc.inc = (float)data_samplerate / k_samplerate;
}

struct sample
{
    int data;
    float scaling;
    uint16_t length;
};

static inline struct sample get_wave_data(const struct data_osc *osc, uint32_t idx)
{
    struct sample s;
    s.length = osc->wfd.length;
    if (bits == 8)
    {
        if (idx >= s.length)
        {
            s.data = 0;
            s.scaling = 1;
            return s;
        }
        int d = osc->wfd.data[idx];
        if (d > 0x7f)
            d = 0x7f - d;
        s.data = d;
        s.scaling = 1 / (float)0x7f;
    }
    else
    {
        s.length /= 2;
        if (idx >= s.length)
        {
            s.data = 0;
            s.scaling = 1;
            return s;
        }
        int d = (osc->wfd.data[idx * 2] << 8) | (osc->wfd.data[idx * 2 + 1]);
        if (d > 0x7fff)
            d = 0x7fff - d;
        s.data = d;
        s.scaling = 1 / (float)0x7fff;
    }
    return s;
}

static inline float data_osc_process(struct data_osc *osc)
{
    uint32_t i = osc->phase;

    const struct sample curr = get_wave_data(osc, i);
    float out = curr.data;
    if (user_params.linint)
    {
        const struct sample next = get_wave_data(osc, i + 1);

        out += (next.data - curr.data) * (osc->phase  - i);
    }

    osc->phase += osc->inc;
    if (osc->phase >= curr.length || osc->phase < 0)
        osc->phase = osc->loopback_idx;

    return out * osc->mix * curr.scaling;
}

static void handle_midi_cc(uint8_t midi_word)
{
    /*
     * Format:
     * [0...3] DATA
     * [4...6] META
     * 
     * META:
     * 0..3 -> sequence number
     * 4 -> start data segment. DATA contains id (see segments.h).
     * others: state machine reset
     * */
    
    static uint32_t data_word;
    static uint8_t segment, expected_seq_num;
    const uint8_t DATA = midi_word & 0xF;
    const uint8_t META = (midi_word >> 4) & 7;
    if (META > 4)
    {
        // Reset
        data_word = segment = expected_seq_num = 0;
    }
    else if (META == 4)
    {
        // Segment start
        segment = DATA;
        if (segment == SEG_WAVE)
        {
            osc.wfd.length = 0;
            set_sample_metadata_defaults();
        }
        data_word = 0;
        expected_seq_num = 0;
    }
    else if (META == expected_seq_num)
    {
        // Segment data
        data_word = (data_word << 4) | DATA;
        if (segment == SEG_SAMPLERATE && META == 3)
        {
            data_samplerate = data_word;
            segment = SEG_NONE;
        }
        if (segment == SEG_BASEFREQ && META == 3 && (data_word & 0xFFFF0000))
        {
            base_freq = data_word / 1e6f;
            segment = SEG_NONE;
        }
        if (segment == SEG_LOOPIDX && META == 3)
        {
            osc.loopback_idx = data_word;
            segment = SEG_NONE;
        }
        if (segment == SEG_BITS && META == 1)
        {
            bits = data_word;
            segment = SEG_NONE;
        }
        if (segment == SEG_WAVE && META % 2 == 1 && osc.wfd.length < DATA_LEN)
        {
            waveform[osc.wfd.length] = data_word;
            osc.wfd.length++;
            data_word = 0;
        }
        expected_seq_num = (expected_seq_num + 1) % 4;
    }
}

void OSC_INIT(uint32_t platform, uint32_t api)
{
    (void) platform;
    (void) api;
    osc.wfd.data = waveform;
    osc.loopback_idx = DATA_LEN;
    set_sample_metadata_defaults();
    flanger_osc.frequency = 4.0f / k_samplerate;
}

void OSC_CYCLE(const user_osc_param_t *const params, int32_t *yn, const uint32_t frames)
{
    const float shape_lfo = q31_to_f32(params->shape_lfo);
    const float delay_read_offset = user_params.flanger_mix * 50 + user_params.flanger_mix * SimpleOscillator_getValue(&flanger_osc, OSC_TRIANGLE) * 50 + shape_lfo * 100;
    update_inc(params);
    if (user_params.reverse)
        osc.inc *= -1;
    if (retrig_counter >= frames)
    {
        retrig_counter -= frames;
        if (retrig_counter < frames)
        {
            osc.phase = 0;
            if (user_params.reverse)
                osc.phase = osc.wfd.length * 8 / bits - osc.phase;
            osc.mix *= user_params.retrig_amp;
            retrig_counter = user_params.retrig;
        }
    }
    flanger_osc.phase += flanger_osc.frequency * frames;
    flanger_osc.phase -= (int)flanger_osc.phase;

    OSC_LOOP(y, yn, frames)
    {
        const float osc_out = data_osc_process(&osc);
        int delay_read_idx = delay_idx - delay_read_offset;
        if (delay_read_idx < 0)
            delay_read_idx += 200;
        const float out = osc_out + delay_buf[delay_read_idx] * -user_params.flanger_mix;
        delay_buf[delay_idx] = osc_out;

        if (++delay_idx >= 200)
            delay_idx = 0;

        *(y++) = safe_f32_to_q31(out);
    }
}

void OSC_NOTEON(const user_osc_param_t *const params)
{
    const float len = osc.wfd.length * 8 / bits;
    osc.phase = (((params->pitch) >> 8) & 3) * user_params.split * len;
    if (user_params.reverse)
        osc.phase = len - osc.phase;
    osc.mix = 1;
    retrig_counter = user_params.retrig;
}

void OSC_NOTEOFF(const user_osc_param_t *const params)
{
    (void) params;
}

void OSC_PARAM(uint16_t index, uint16_t value)
{
    switch (index)
    {
        case USER_PARAM__Split__idx:
            user_params.split = value / 100.0f;
            break;
        case USER_PARAM__Track_kb__idx:
            user_params.kb_track = 1 - value;
            break;
        case USER_PARAM__Interpolation__idx:
            user_params.linint = 1 - value;
            break;
        case USER_PARAM__Reverse__idx:
            user_params.reverse = value;
            break;
        case USER_PARAM__Retrig__idx:
            user_params.retrig = value * 480;
            break;
        case USER_PARAM__Retrig_amp__idx:
            user_params.retrig_amp = value / 100.0f;
            break;
        case k_user_osc_param_shape:
            user_params.flanger_mix = param_val_to_f32(value);
            break;
        case k_user_osc_param_shiftshape:
            //noise_mix = param_val_to_f32(value);
            handle_midi_cc((value >> 3) & 0x7f);
            break;
        default:
            break;
    }
}
