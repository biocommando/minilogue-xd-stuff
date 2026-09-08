#include "usermodfx.h"
#include "combined_waveforms.h"

static float gain, vol, env, osc_inc = 0;
static const uint8_t *pattern;
#define SHORT_BLIP_LENGTH 3000 // 62.5 ms
#define BLIP_POLARITY_MASK_HI 0x20 // 750 Hz
#define BLIP_POLARITY_MASK_LO 0x40 // 325 Hz
#define BLIP_MUTE_MASK 0x1000 // 85.33 ms
#define BLIP_AMPLITUDE 0.2f
static uint16_t blip_counter = 0;
static uint8_t blip_polarity_mask;

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

#define N_PATTERNS 17

static uint8_t patterns[N_PATTERNS][N_STEPS + 1] = {
    {8, 0, 0, 0, 8, 0, 0, 0, 8, 0, 0, 0, 8, 0, 0, 0, 4}, // non-accented metronome
    {21, 4, 22, 4, 21, 5, 22, 20, 21, 4, 22, 4, 21, 38, 54, 22, 2}, // rock 1
    {5, 52, 6, 52, 5, 1, 6, 52, 5, 20, 6, 20, 36, 1, 38, 70, 2}, // rock 2
    {5, 4, 20, 4, 6, 4, 20, 9, 68, 69, 118, 68, 4, 9, 54, 36, 2}, // slow beat
    {17, 4, 19, 4, 17, 4, 19, 4, 17, 4, 19, 4, 17, 4, 19, 21, 2}, // 4 on the floor
    {65, 20, 3, 20, 1, 20,  3, 20, 65, 20, 3, 20, 1, 20, 3, 53, 2}, // pop 2
    {113, 40, 0, 136, 113, 0, 136, 0, 115, 0, 136, 0, 113, 136, 0, 136, 4}, // pop 1
    {21, 1, 4, 40, 22, 72, 5, 24, 4, 8, 5, 88, 22, 40, 22, 88, 4}, // funky rock
    {21, 100, 20, 100, 22, 100, 21, 100, 20, 100, 21, 100, 54, 52, 54, 54, 4}, // pitch var hh beat
    {21, 36, 148, 36, 148, 36, 148, 164, 86, 36, 148, 36, 148, 36, 148, 164, 4}, // fast hats, slow kick snare
    {21, 33, 136, 1, 22, 0, 1, 136, 21, 136, 1, 0, 22, 8, 1, 136, 4}, // straight rock with percs
    {21, 8, 4, 1, 6, 40, 21, 2, 5, 8, 21, 40, 6, 162, 69, 40, 4}, // funky rock with percs
    {21, 36, 44, 36, 22, 36, 13, 36, 20, 36, 13, 36, 22, 36, 44, 38, 4}, // rocky hihats
    {21, 0, 5, 0, 22, 0, 4, 34, 4, 130, 5, 1, 22, 0, 4, 170, 4}, // amen
    {101, 4, 101, 4, 22, 4, 4, 38, 4, 134, 101, 38, 22, 101, 4, 134, 4}, // funky
    {51, 0, 164, 0, 162, 0, 164, 0, 162, 0, 164, 0, 162, 0, 164, 0, 4}, // metronome 2
    {30, 0, 8, 0, 8, 0, 8, 0, 28, 0, 8, 0, 8, 0, 8, 0, 2}, // metronome
};

// Sample playback

struct compressed_osc
{
    struct waveform wf;
    float phase;
    float mix;
};

static __sdram struct compressed_osc osc[4];

__fast_inline int8_t compressed_osc_get(struct compressed_osc *osc)
{
    const uint16_t i = osc->phase;
    if (i >= osc->wf.length)
    {
        osc->phase = 0x8000;
        return 0;
    }
    const uint16_t ai = i / 5;
    const uint8_t wi = i % 5;
    const uint16_t word = osc->wf.data[ai];
    const int8_t val = (word >> (wi * 3)) & 0x7;
    if (word & 0x8000)
        return -val;
    return val;
}

static __sdram int8_t looper[128 * 1024 - sizeof(osc)];
static uint32_t looper_idx;
#define LOOPER_IDLE 0
#define LOOPER_PLAY 1
#define LOOPER_REC 2
static uint8_t looper_mode;
// Samplerate conversion is calculated from ratio: looper_sr_ratio / 4800
static uint16_t looper_sr_ratio;

static uint32_t seq_sample = 0, next_seq_trig = 0, seq_pos = 0,
    tempo = 0, step_len = 0;

static void set_step_length(uint32_t _tempo)
{
  step_len = 48000 * 600 / _tempo / pattern[STEP_DIV_IDX];
  // Adaptive samplerate between 87.9 ... 175.8 bpm;
  // limited to 24 ... 48 kHz
  if (_tempo > 1758)
    looper_sr_ratio = 4800;
  else if (_tempo < 879)
    looper_sr_ratio = 2400;
  else
    looper_sr_ratio = 27297 * _tempo / 10000;
}

__fast_inline float blip()
{
    if (blip_counter > 0)
    {
        if ((blip_counter-- & BLIP_MUTE_MASK) == 0)
             return blip_counter & blip_polarity_mask ? BLIP_AMPLITUDE : -BLIP_AMPLITUDE;
    }
    return 0;
}

static void restart_playback()
{
    seq_sample = 0;
    looper_idx = 0;
    seq_pos = 0;
    next_seq_trig = 1;
}

void MODFX_INIT(uint32_t platform, uint32_t api)
{
    (void) platform;
    (void) api;
    for (uint32_t i = 0; i < 4; i++)
    {
        osc[i].wf = get_waveform(i);
        // Compensate compression here to keep the decompression
        // as purely integer maths
        osc[i].mix = 2.0f / 7.0f;
    }
    osc[WAVEFORM_ID_hhc].mix = 0.67f / 7.0f;
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

void MODFX_PROCESS(const float * __restrict x, float * __restrict y,
                   const float *sub_xn, float *sub_yn, uint32_t frames)
{
    (void) sub_xn;
    (void) sub_yn;

    const uint32_t new_tempo = fx_get_bpm();
    if (new_tempo != tempo)
    {
        set_step_length(new_tempo);
        restart_playback();
        tempo = new_tempo;
    }

    for (uint32_t i = 0; i < frames; i++)
    {
        const float input = *(x++);
        float output = 0;
        if (wait_thd_cross != WAIT_THD_CROSS_IDLE)
        {
            const float abs_input = input > 0 ? input : -input;
            if (wait_thd_cross == WAIT_THD_CROSS_ACTIVE)
            {
                if (abs_input > seq_trig_thd)
                {
                    wait_thd_cross = WAIT_THD_CROSS_IDLE;
                }
                // a bit dirty way to do it but this avoids duplicating the
                // output buffer accumulation code
                goto loop_end;
            }
            if (wait_thd_cross == WAIT_THD_CROSS_ARMED)
            {
                // try to track the actual noise floor by letting any
                // accidental played notes die out eventually.
                // 0.9999: 145 ms for 50 % signal strength, 480 ms for 10%
                seq_trig_thd *= 0.9999f;
                if (abs_input > seq_trig_thd)
                    seq_trig_thd = abs_input;
            }
        }
        seq_sample++;
        if (seq_sample == next_seq_trig)
        {
            if (seq_pos == 16)
            {
                restart_playback();
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
        // No interpolation etc., we're already working really close
        // to size limits.
        const uint32_t looper_idx_adaptive_sr = looper_idx * looper_sr_ratio / 4800;
        if (looper_idx_adaptive_sr < sizeof(looper))
        {
            if (looper_mode == LOOPER_PLAY && gain >= 0.001f)
            {
                output += looper[looper_idx_adaptive_sr] / 127.0f;
            }
            else if (looper_mode == LOOPER_REC)
            {
                looper[looper_idx_adaptive_sr] = 127 * (input);
            }
            looper_idx++;
        }
loop_end:
        *(y++) = output + input;
        *(y++) = output + *(x++);
    }
    if (halt_timeout_counter > frames)
        halt_timeout_counter -= frames;
    else
        halt_counter = halt_timeout_counter = 0;
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
            // This will delegate the sequence restart to the processing
            // hook. Resetting sequence here proved to be unreliable;
            // sometimes the looper began playing at wrong offset.
            tempo = 0;
            blip_counter = SHORT_BLIP_LENGTH;
            blip_polarity_mask = new_p[STEP_DIV_IDX] == 4 ? BLIP_POLARITY_MASK_HI : BLIP_POLARITY_MASK_LO;
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
            restart_playback();
            if (wait_thd_cross == WAIT_THD_CROSS_ARMED)
            {
                wait_thd_cross = WAIT_THD_CROSS_ACTIVE;
                blip_counter = 0;
                seq_trig_thd *= 2;
            }
        }
        else if (gain >= 0.001f && v < 0.001f)
        {
            if (halt_timeout_counter == 0 || wait_thd_cross != WAIT_THD_CROSS_IDLE)
            {
                halt_timeout_counter = 48000;
                halt_counter = 0;
                wait_thd_cross = WAIT_THD_CROSS_IDLE;
            }
            halt_counter++;
            if (halt_counter == 2)
            {
                halt_counter = 0;
                halt_timeout_counter = 0;
                wait_thd_cross = WAIT_THD_CROSS_ARMED;
                // 0:beep, 1:rest, 2:beep, 3:rest, 4:beep
                blip_counter = BLIP_MUTE_MASK * 5;
                blip_polarity_mask = BLIP_POLARITY_MASK_HI;
            }
        }
        gain = v;
    }
}
