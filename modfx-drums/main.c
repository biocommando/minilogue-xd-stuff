#include "usermodfx.h"
#include "combined_waveforms.h"

static float gain = 1;
static const uint8_t *pattern;

static uint8_t patterns[][16] = {
    {5,4,6,4,5,4,6,4,5,4,6,4,5,4,6,4},
    {5,4,6,4,4,1,6,4,5,4,6,4,4,1,6,4},
    {5,4,4,4,6,4,4,1,4,5,6,4,4,1,6,4},
    {1,4,3,4,1,4,3,4,1,4,3,4,1,4,3,4},
    {8,0,4,0,4,0,4,0,8,0,4,0,4,0,4,0},
};

// Sample playback

struct compressed_osc
{
    const uint16_t *data;
    uint16_t data_len;
    float phase;
    float inc;
    float mix;
};

static struct compressed_osc osc[4];

static inline float compressed_osc_get(const struct compressed_osc *osc)
{
    int i = osc->phase;
    int ai = i / 5;
    int wi = i % 5;
    if (ai >= osc->data_len || ai < 0)
        return 0;
    uint16_t word = osc->data[ai];
    float s = ((int) ((word >> (wi * 3)) & 0x7)) / 7.0f;
    return (word & 0x8000) ? -s : s;
}


static uint32_t seq_sample = 0, next_seq_trig = 0, seq_pos = 0,
    tempo = 0, step_len = 0, seq_len = 0;

void MODFX_INIT(uint32_t platform, uint32_t api)
{
    (void) platform;
    (void) api;
    for (int i = 0; i < 4; i++)
    {
        osc[i].data = get_waveform(i, &osc[i].data_len);
        osc[i].phase = osc[i].data_len * 5;
        osc[i].inc = 44100.0f / 48000.0f;
        osc[i].mix = 1;
    }
    osc[WAVEFORM_ID_hhc].mix = 0.2;
    pattern = patterns[0];
}

void MODFX_PROCESS(const float *main_xn, float *main_yn, const float *sub_xn, float *sub_yn, uint32_t frames)
{
    (void) sub_xn;
    (void) sub_yn;

    const uint32_t new_tempo = fx_get_bpm();
    if (new_tempo != tempo)
    {
        step_len = 48000 * 600 / new_tempo / 2;
        seq_len = 16 * step_len;
        seq_sample = seq_pos * step_len;
        next_seq_trig = (seq_pos + 1) * step_len;
        if (tempo == 0)
            next_seq_trig = 1;
        tempo = new_tempo;
    }

    const float *__restrict x = (const float *) main_xn;
    float *__restrict y = (float *) main_yn;

    for (uint32_t i = 0; i < frames; i++)
    {
        seq_sample++;
        if (seq_sample == next_seq_trig)
        {
            if (seq_pos == 16)
            {
                seq_sample = 0;
                seq_pos = 0;
            }
            next_seq_trig = seq_sample + step_len;
            const uint8_t triggers = pattern[seq_pos];
            if (triggers & 1)
                osc[WAVEFORM_ID_bd].phase = 0;
            if (triggers & 2)
                osc[WAVEFORM_ID_sd].phase = 0;
            if (triggers & 4)
                osc[WAVEFORM_ID_hhc].phase = 0;
            if (triggers & 8)
                osc[WAVEFORM_ID_rim].phase = 0;
            seq_pos++;
        }
        
        float output = 0;
        for (int i = 0; i < 4; i++)
        {
            osc[i].phase += osc[i].inc;
            output += compressed_osc_get(&osc[i]) * osc[i].mix;
        }
        output *= gain;
        *(y++) = output + *(x++);
        *(y++) = output + *(x++);
    }
}

void MODFX_PARAM(uint8_t index, int32_t value)
{
    const float v = q31_to_f32(value);
    if (index == k_user_modfx_param_time)
    {
        pattern = patterns[(int)(5 * 0.99 * v)];
    }
    else if (index == k_user_modfx_param_depth)
    {
        if (gain < 0.001f && v >= 0.001f)
        {
            seq_pos = 0;
            seq_sample = 0;
            next_seq_trig = step_len;
        }
        gain = v;
    }
}
