#include "usermodfx.h"
#include "combined_waveforms.h"

static float gain = 1, vol = 1, env = 1, osc_inc = 0;
static const uint8_t *pattern;
static uint16_t blip_counter = 0;

#ifdef MODFX_DRUMS_DEBUG
    void set_pattern(const uint8_t *p) { pattern = p; }
#endif

#define N_STEPS 16
#define STEP_DIV_IDX N_STEPS

#define N_PATTERNS 16

static uint8_t patterns[N_PATTERNS][N_STEPS + 1] = {
    {21, 4, 22, 4, 21, 5, 22, 20, 21, 4, 22, 4, 21, 38, 54, 22, 2},
    {5, 52, 6, 52, 5, 1, 6, 52, 5, 20, 6, 20, 36, 1, 38, 70, 2},
    {5, 4, 20, 4, 6, 4, 20, 9, 68, 69, 118, 68, 4, 9, 54, 36, 2},
    {113, 40, 0, 136, 113, 0, 136, 0, 115, 0, 136, 0, 113, 136, 0, 136, 4},
    {65, 20, 3, 20, 1, 20, 3, 20, 65, 20, 3, 20, 1, 20, 3, 53, 2},
    {21, 65, 5, 33, 23, 65, 5, 33, 21, 65, 5, 33, 87, 99, 7, 51, 4},
    {21, 1, 4, 40, 22, 72, 5, 24, 4, 8, 5, 88, 22, 40, 22, 88, 4},
    {21, 100, 20, 100, 22, 100, 21, 100, 20, 100, 21, 100, 54, 52, 54, 54, 4},
    {21, 36, 148, 36, 148, 36, 148, 164, 86, 36, 148, 36, 148, 36, 148, 164, 4},
    {21, 33, 136, 1, 22, 0, 1, 136, 21, 136, 1, 0, 22, 8, 1, 136, 4},
    {21, 8, 4, 1, 6, 40, 21, 2, 5, 8, 21, 40, 6, 162, 69, 40, 4},
    {21, 36, 44, 36, 22, 36, 13, 36, 20, 36, 13, 36, 22, 36, 44, 38, 4},
    {21, 0, 5, 0, 22, 0, 4, 34, 4, 130, 5, 1, 22, 0, 4, 170, 4}, // amen
    {101, 4, 101, 4, 22, 4, 4, 38, 4, 134, 101, 38, 22, 101, 4, 134, 4}, // funky
    {51, 0, 164, 0, 162, 0, 164, 0, 162, 0, 164, 0, 162, 0, 164, 0, 4}, // metronome 2
    {28, 0, 8, 0, 8, 0, 8, 0, 28, 0, 8, 0, 8, 0, 8, 0, 2}, // metronome
};

// Sample playback

struct compressed_osc
{
    const uint16_t *data;
    uint16_t data_len;
    float phase;
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

static inline void set_step_length(uint32_t _tempo)
{
  step_len = 48000 * 600 / _tempo / pattern[STEP_DIV_IDX];
  seq_len = N_STEPS * step_len;
}

void MODFX_INIT(uint32_t platform, uint32_t api)
{
    (void) platform;
    (void) api;
    for (int i = 0; i < 4; i++)
    {
        osc[i].data = get_waveform(i, &osc[i].data_len);
        osc[i].phase = osc[i].data_len * 5;
        osc[i].mix = 1;
    }
    osc[WAVEFORM_ID_hhc].mix = 0.33;
    pattern = patterns[0];
}

#define TRIG_MASK_BD 1
#define TRIG_MASK_SD 2
#define TRIG_MASK_HH 4
#define TRIG_MASK_RIM 8
#define TRIG_MASK_ACCENT 16
#define TRIG_MASK_SHORT_DECAY 32
#define TRIG_MASK_ALT_PITCH 64
#define TRIG_MASK_PLAY_OFFSET 128
#define SHORT_DECAY_INIT_VAL 0.9997f
#define ALT_PITCH_RATIO 0.7f
#define PLAY_OFFSET_SAMPLES 1000

void MODFX_PROCESS(const float *main_xn, float *main_yn, const float *sub_xn, float *sub_yn, uint32_t frames)
{
    (void) sub_xn;
    (void) sub_yn;

    const uint32_t new_tempo = fx_get_bpm();
    if (new_tempo != tempo)
    {
        set_step_length(new_tempo);
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
            uint16_t phase_offset = 0;
            if (triggers & TRIG_MASK_PLAY_OFFSET)
                phase_offset = PLAY_OFFSET_SAMPLES;
            if (triggers & TRIG_MASK_BD)
                osc[WAVEFORM_ID_bd].phase = phase_offset;
            if (triggers & TRIG_MASK_SD)
                osc[WAVEFORM_ID_sd].phase = phase_offset;
            if (triggers & TRIG_MASK_HH)
                osc[WAVEFORM_ID_hhc].phase = phase_offset;
            if (triggers & TRIG_MASK_RIM)
                osc[WAVEFORM_ID_rim].phase = phase_offset;
            vol = gain * 0.5;
            if (triggers & TRIG_MASK_ACCENT)
                vol *= 2;
            env = 1;
            if (triggers & TRIG_MASK_SHORT_DECAY)
                env = SHORT_DECAY_INIT_VAL;
            osc_inc = 44100.0f / 48000.0f;
            if (triggers & TRIG_MASK_ALT_PITCH)
                osc_inc *= ALT_PITCH_RATIO;
            seq_pos++;
        }
        
        float output = 0;
        for (int i = 0; i < 4; i++)
        {
            osc[i].phase += osc_inc;
            output += compressed_osc_get(&osc[i]) * osc[i].mix;
        }
        output *= vol;
        vol *= env;
        if (blip_counter > 0)
        {
            output += blip_counter & 63 ? 0.3 : -0.3;
            blip_counter--;
        }
        *(y++) = output + *(x++);
        *(y++) = output + *(x++);
    }
}

static void reset_seq()
{
    seq_pos = 0;
    seq_sample = 0;
    next_seq_trig = step_len;
}

void MODFX_PARAM(uint8_t index, int32_t value)
{
    const float v = q31_to_f32(value);
    if (index == k_user_modfx_param_time)
    {
        const uint8_t * new_p = patterns[(int)(N_PATTERNS * 0.99 * v)];
        if (tempo)
          set_step_length(tempo);
        if (new_p != pattern)
        {
            reset_seq();
            blip_counter = 3000;
        }
        pattern = new_p;
    }
    else if (index == k_user_modfx_param_depth)
    {
        if (gain < 0.001f && v >= 0.001f)
        {
            reset_seq();
        }
        gain = v;
    }
}
