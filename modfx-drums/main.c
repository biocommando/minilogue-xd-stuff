#include "usermodfx.h"
#include "combined_waveforms.h"

static float gain = 1, vol = 1, env = 1, osc_inc = 0;
static const uint8_t *pattern;
#define SHORT_BLIP_LENGTH 3000 // 62.5 ms
#define BLIP_POLARITY_MASK 0x20 // 750 Hz
#define BLIP_MUTE_MASK 0x1000 // 85.33 ms
#define BLIP_AMPLITUDE 0.2f
static uint16_t blip_counter = 0;

#define WAIT_THD_CROSS_IDLE 0
#define WAIT_THD_CROSS_ARMED 1
#define WAIT_THD_CROSS_ACTIVE 2

static uint8_t halt_counter = 0, wait_thd_cross = WAIT_THD_CROSS_IDLE;
static uint16_t halt_timeout_counter = 0;
static float seq_trig_thd = 0;

#ifdef MODFX_DRUMS_DEBUG
    void set_pattern(const uint8_t *p)
    {
        pattern = p;
        // disable "blip" at the start when overriding the pattern directly.
        // the counter is set by the fxsound init code in MODFX_PARAM.
        blip_counter = 0;
    }
#endif

#define N_STEPS 16
#define STEP_DIV_IDX N_STEPS

#define N_PATTERNS 16

static uint8_t patterns[N_PATTERNS][N_STEPS + 1] = {
    {21, 4, 22, 4, 21, 5, 22, 20, 21, 4, 22, 4, 21, 38, 54, 22, 2}, // rock 1
    {5, 52, 6, 52, 5, 1, 6, 52, 5, 20, 6, 20, 36, 1, 38, 70, 2}, // rock 2
    {5, 4, 20, 4, 6, 4, 20, 9, 68, 69, 118, 68, 4, 9, 54, 36, 2}, // slow beat
    {113, 40, 0, 136, 113, 0, 136, 0, 115, 0, 136, 0, 113, 136, 0, 136, 4}, // pop 1
    {65, 20, 3, 20, 1, 20,  3, 20, 65, 20, 3, 20, 1, 20, 3, 53, 2}, // pop 2
    //{21, 2, 5, 34, 21, 2, 5, 34, 21, 2, 5, 34, 87, 34, 7, 50, 2}, // humppa
    {21, 1, 4, 40, 22, 72, 5, 24, 4, 8, 5, 88, 22, 40, 22, 88, 4}, // funky rock
    {21, 100, 20, 100, 22, 100, 21, 100, 20, 100, 21, 100, 54, 52, 54, 54, 4}, // pitch var hh beat
    {21, 36, 148, 36, 148, 36, 148, 164, 86, 36, 148, 36, 148, 36, 148, 164, 4}, // fast hats, slow kick snare
    {21, 33, 136, 1, 22, 0, 1, 136, 21, 136, 1, 0, 22, 8, 1, 136, 4}, // straight rock with percs
    {21, 8, 4, 1, 6, 40, 21, 2, 5, 8, 21, 40, 6, 162, 69, 40, 4}, // funky rock with percs
    {21, 36, 44, 36, 22, 36, 13, 36, 20, 36, 13, 36, 22, 36, 44, 38, 4}, // rocky hihats
    {17, 4, 19, 4, 17, 4, 19, 4, 17, 4, 19, 4, 17, 4, 19, 21, 2}, // 4 on the floor
    {21, 0, 5, 0, 22, 0, 4, 34, 4, 130, 5, 1, 22, 0, 4, 170, 4}, // amen
    {101, 4, 101, 4, 22, 4, 4, 38, 4, 134, 101, 38, 22, 101, 4, 134, 4}, // funky
    {51, 0, 164, 0, 162, 0, 164, 0, 162, 0, 164, 0, 162, 0, 164, 0, 4}, // metronome 2
    {30, 0, 8, 0, 8, 0, 8, 0, 28, 0, 8, 0, 8, 0, 8, 0, 2}, // metronome
};

static __sdram int8_t looper[128 * 1024 - 256];
static uint32_t looper_idx;
#define LOOPER_IDLE 0
#define LOOPER_PLAY 1
#define LOOPER_REC 2
static uint8_t looper_mode;

// Sample playback

struct compressed_osc
{
    const uint16_t *data;
    uint16_t data_len;
    float phase;
    float mix;
};

static __sdram struct compressed_osc osc[4];

static inline float compressed_osc_get(struct compressed_osc *osc)
{
    const uint16_t i = osc->phase;
    const uint16_t ai = i / 5;
    const uint8_t wi = i % 5;
    if (ai >= osc->data_len)
    {
        osc->phase = 0x8000;
        return 0;
    }
    const uint16_t word = osc->data[ai];
    const float s = ((int) ((word >> (wi * 3)) & 0x7)) / 7.0f;
    return (word & 0x8000) ? -s : s;
}


static uint32_t seq_sample = 0, next_seq_trig = 0, seq_pos = 0,
    tempo = 0, step_len = 0, seq_len = 0;

static inline void set_step_length(uint32_t _tempo)
{
  step_len = 48000 * 600 / _tempo / pattern[STEP_DIV_IDX];
  seq_len = N_STEPS * step_len;
}

static inline float blip()
{
    if (blip_counter > 0)
    {
        if ((blip_counter-- & BLIP_MUTE_MASK) == 0)
             return blip_counter & BLIP_POLARITY_MASK ? BLIP_AMPLITUDE : -BLIP_AMPLITUDE;
    }
    return 0;
}

void MODFX_INIT(uint32_t platform, uint32_t api)
{
    (void) platform;
    (void) api;
    for (uint32_t i = 0; i < 4; i++)
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
        seq_sample = 0;
        looper_idx = 0;
        next_seq_trig = (seq_pos + 1) * step_len;
        if (tempo == 0)
            next_seq_trig = 1;
        tempo = new_tempo;
    }

    const float *__restrict x = (const float *) main_xn;
    float *__restrict y = (float *) main_yn;

    for (uint32_t i = 0; i < frames; i++)
    {
        float output = 0;
        if (wait_thd_cross == WAIT_THD_CROSS_ACTIVE)
        {
            if (*x > seq_trig_thd || *x < -seq_trig_thd)
            {
                wait_thd_cross = 0;
            }
            *(y++) = *(x++);
            *(y++) = *(x++);
            continue;
        }
        seq_sample++;
        if (seq_sample == next_seq_trig)
        {
            if (seq_pos == 16)
            {
                looper_idx = 0;
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

        for (int i = 0; i < 4; i++)
        {
            osc[i].phase += osc_inc;
            output += compressed_osc_get(&osc[i]) * osc[i].mix;
        }
        output *= vol;
        output += blip();
        vol *= env;
        if (looper_idx < sizeof(looper) * 2)
        {
            // 32 kHz sampling rate will allow recording 4 bar length
            // loop for all tempos down to 58.8 bpm.
            // No interpolation etc., we're already working really close
            // to size limits.
            const uint32_t actual_idx = looper_idx * 32 / 48;
            if (looper_mode == LOOPER_PLAY && gain > 0.001f)
            {
                output += looper[actual_idx] / 127.0f;
            }
            else if (looper_mode == LOOPER_REC)
            {
                looper[actual_idx] = 127 * (*x);
            }
            looper_idx++;
        }
        if (wait_thd_cross == WAIT_THD_CROSS_ARMED && *x > seq_trig_thd)
            seq_trig_thd = *x;
        /*
        // Size optimization... I think positive peak should be
        // enough in most cases, we use both polarities for
        // triggering though.
        else if (*x < -seq_trig_thd)
            seq_trig_thd = -*x;
        */
        *(y++) = output + *(x++);
        *(y++) = output + *(x++);
    }
    if (halt_timeout_counter > frames)
        halt_timeout_counter -= frames;
    else
        halt_counter = halt_timeout_counter = 0;
}

static void reset_seq()
{
    looper_idx = 0;
    seq_pos = 0;
    seq_sample = 0;
    next_seq_trig = 1;
}

void MODFX_PARAM(uint8_t index, int32_t value)
{
    const float v = q31_to_f32(value);
    if (index == k_user_modfx_param_time)
    {
        const uint8_t pattern_idx = (int)(N_PATTERNS * 0.99 * v);
        const uint8_t * new_p = patterns[pattern_idx];
        if (tempo)
          set_step_length(tempo);
        if (new_p != pattern)
        {
            reset_seq();
            blip_counter = SHORT_BLIP_LENGTH;
        }
        pattern = new_p;
        if (pattern_idx == N_PATTERNS - 1)
            looper_mode = LOOPER_REC;
        else if (looper_mode == LOOPER_REC)
            looper_mode = LOOPER_PLAY;
        if (pattern_idx == 0)
            looper_mode = LOOPER_IDLE;
        seq_trig_thd = 0;
        wait_thd_cross = WAIT_THD_CROSS_IDLE;
    }
    else if (index == k_user_modfx_param_depth)
    {
        if (gain < 0.001f && v >= 0.001f)
        {
            reset_seq();
            if (wait_thd_cross == WAIT_THD_CROSS_ARMED)
            {
                wait_thd_cross = WAIT_THD_CROSS_ACTIVE;
                seq_trig_thd *= 2;
            }
        }
        else if (gain >= 0.001f && v < 0.001f)
        {
            if (halt_timeout_counter == 0 || wait_thd_cross != WAIT_THD_CROSS_IDLE)
            {
                halt_timeout_counter = 48000;
                halt_counter = 0;
                wait_thd_cross = 0;
            }
            halt_counter++;
            if (halt_counter == 2)
            {
                halt_counter = 0;
                halt_timeout_counter = 0;
                wait_thd_cross = WAIT_THD_CROSS_ARMED;
                // 0:beep, 1:rest, 2:beep, 3:rest, 4:beep
                blip_counter = BLIP_MUTE_MASK * 5;
            }
        }
        gain = v;
    }
}
