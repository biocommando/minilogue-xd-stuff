#include "userosc.h"
#include "manifest_params.h"
#include "stuff_util.h"

#define MIDDLE_C_FREQ_HZ 261.6256

struct waveform_data
{
    const int8_t *data;
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
static float base_freq = MIDDLE_C_FREQ_HZ;
static uint16_t data_samplerate = 16000;
#define DATA_LEN 30000
static int8_t waveform[DATA_LEN];

static inline void update_inc(const user_osc_param_t *const params)
{
     osc.inc = osc_w0f_for_note((params->pitch) >> 8, params->pitch & 0xFF) / base_freq * data_samplerate;
}

void OSC_INIT(uint32_t platform, uint32_t api)
{
    (void) platform;
    (void) api;
    osc.wfd.data = waveform;
    osc.loopback_idx = DATA_LEN;
    osc.mix = 1;
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

static void handle_midi_cc(uint8_t midi_word)
{
    /*
     * Format:
     * [0...3] DATA
     * [4...7] META
     * 
     * META:
     * 0..3 -> sequence number
     * 4 -> start data segment. DATA contains id:
     *  * 1: samplerate (16 bit int)
     *  * 2: base frequency (32 bit int)
     *  * 3: loop index (16 bit int)
     *  * 4: data (8 bit signed ints)
     *  * others: nothing selected
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
        if (segment == 3)
        {
            osc.wfd.length = 0;
        }
        data_word = 0;
        expected_seq_num = 0;
    }
    else if (META == expected_seq_num)
    {
        // Segment data
        data_word = (data_word << 4) | DATA;
        if (segment == 1 && META == 3)
        {
            data_samplerate = data_word;
            segment = 0;
        }
        if (segment == 2 && META == 3 && (data_word & 0xFFFF0000))
        {
            base_freq = data_word / 1e6f;
            segment = 0;
        }
        if (segment == 3 && META == 3)
        {
            osc.loopback_idx = data_word;
            segment = 0;
        }
        if (segment == 4 && META % 2 == 1 && osc.wfd.length < DATA_LEN)
        {
            int d = data_word;
            if (d > 127)
                d = 127 - d;
            waveform[osc.wfd.length] = d;
            osc.wfd.length++;
            data_word = 0;
        }
        expected_seq_num = (expected_seq_num + 1) % 4;
    }
}

void OSC_CYCLE(const user_osc_param_t *const params, int32_t *yn, const uint32_t frames)
{
    update_inc(params);

    OSC_LOOP(y, yn, frames)
    {
        float out = data_osc_process(&osc);

        *(y++) = safe_f32_to_q31(out);
    }
}

void OSC_NOTEON(const user_osc_param_t *const params)
{
    (void) params;
    osc.phase = 0;
}

void OSC_NOTEOFF(const user_osc_param_t *const params)
{
    (void) params;
}

void OSC_PARAM(uint16_t index, uint16_t value)
{
    switch (index)
    {
        case k_user_osc_param_shape:
            //wt_pos = param_val_to_f32(value);
            break;
        case k_user_osc_param_shiftshape:
            //noise_mix = param_val_to_f32(value);
            handle_midi_cc((value >> 3) & 0x7f);
            break;
        default:
            break;
    }
}
