#include "userrevfx.h"
#include "dataoscsrc.h"
#include <math.h>

static float gain, vol, env, osc_inc;
static const uint8_t *pattern;
#define SHORT_BLIP_LENGTH 3000 // 62.5 ms
#define BLIP_POLARITY_MASK_HI 0x20 // 750 Hz
#define BLIP_POLARITY_MASK_LO 0x40 // 325 Hz
#define BLIP_MUTE_MASK 0x1000 // 85.33 ms
#define BLIP_AMPLITUDE 0.2f
static uint16_t blip_counter;
static uint8_t blip_polarity_mask;

enum WaitThdCrossState {
    WAIT_THD_CROSS_IDLE,
    WAIT_THD_CROSS_ARMED,
    WAIT_THD_CROSS_ACTIVE
};
static enum WaitThdCrossState wait_thd_cross;
static uint8_t halt_counter, halt_status;
static uint16_t halt_timeout_counter;
static float seq_trig_thd;

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

struct data_osc
{
    struct waveform_data wfd;
    // Sample player index
    float phase;
    float mix;
};

static inline float data_osc_process(struct data_osc *osc, float inc)
{
    uint32_t i = osc->phase;
    if (i >= osc->wfd.length)
        return 0;

    const float out = osc->wfd.data[i];

    osc->phase += inc;

    return out * osc->mix;
}

static struct data_osc osc[4];
static uint8_t alt_drums;

#define LOOPER_LEN 0x80000
static __sdram int16_t looper[LOOPER_LEN * 2];
static uint32_t looper_idx;
enum LooperMode {
    LOOPER_IDLE, LOOPER_PLAY, LOOPER_REC
};
static enum LooperMode looper_mode;
static float looper_vol;

static uint16_t seq_sample, step_len;
static volatile uint16_t tempo;
static uint8_t seq_pos;
static uint16_t step_len_error, step_len_error_acc;

static inline void set_step_length(uint32_t _tempo)
{
  step_len_error = (48000 * 600 / pattern[STEP_DIV_IDX]) % _tempo;
  step_len_error_acc = 0;
  step_len = 48000 * 600 / _tempo / pattern[STEP_DIV_IDX];
  tempo = _tempo;
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

static void set_drum_waveforms()
{
    if (alt_drums)
    {    
        osc[WAVEFORM_ID_bd].wfd = get_alt_kick_waveform();
        osc[WAVEFORM_ID_sd].wfd = get_alt_snare_waveform();
        osc[WAVEFORM_ID_hhc].wfd = get_alt_hat_waveform();
        osc[WAVEFORM_ID_rim].wfd = get_alt_rim_waveform();
    }
    else
    {
        osc[WAVEFORM_ID_bd].wfd = get_kick_waveform();
        osc[WAVEFORM_ID_sd].wfd = get_snare_waveform();
        osc[WAVEFORM_ID_hhc].wfd = get_hat_waveform();
        osc[WAVEFORM_ID_rim].wfd = get_rim_waveform();
    }
    
}

void REVFX_INIT(uint32_t platform, uint32_t api)
{
    (void) platform;
    (void) api;
    for (uint32_t i = 0; i < 4; i++)
    {
        osc[i].mix = 1;
    }
    set_drum_waveforms();
    osc[WAVEFORM_ID_hhc].mix = 0.333f;
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

void REVFX_PROCESS(float *x, uint32_t frames)
{
    const uint32_t new_tempo = fx_get_bpm();
    if (new_tempo != tempo)
    {
        set_step_length(new_tempo);
        seq_sample = step_len - 1;
        seq_pos = N_STEPS - 1;
    }

    for (uint32_t i = 0; i < frames; i++)
    {
        const float input0 = x[i * 2];
        const float input1 = x[i * 2 + 1];
        if (wait_thd_cross != WAIT_THD_CROSS_IDLE)
        {
            const float abs_input = fabsf(input0);
            if (wait_thd_cross == WAIT_THD_CROSS_ACTIVE)
            {
                if (abs_input > seq_trig_thd)
                {
                    wait_thd_cross = WAIT_THD_CROSS_IDLE;
                }
                x[i * 2] = input0;
                x[i * 2 + 1] = input1;
                continue;
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
        if (++seq_sample >= step_len)
        {
            seq_sample = 0;

            const uint32_t corr = step_len_error + step_len_error_acc;
            step_len_error_acc = corr % new_tempo;
            if (corr >= new_tempo)
                seq_sample--; // will overflow... but the trigger condition should still work correctly
            if (++seq_pos == N_STEPS)
            {
                seq_pos = 0;
                // reset looper rec/playback on first beat
                looper_idx = 0;
            }
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
            osc_inc = DATA_SAMPLERATE / 48000.0f;
            if (triggers & TRIG_MASK_ALT_PITCH)
                osc_inc *= ALT_PITCH_RATIO;
        }
        float output0 = 0;
        for (int i = 0; i < 4; i++)
        {
            output0 += data_osc_process(&osc[i], osc_inc);
        }
        output0 *= 1 / 127.0f;
        output0 *= vol;
        output0 += blip();
        vol *= env;
        float output1 = output0;
        if (looper_idx < LOOPER_LEN)
        {
            if (looper_mode == LOOPER_PLAY && !halt_status)
            {
                const float f = looper_vol / 16384.0f;
                output0 += looper[looper_idx * 2] * f;
                output1 += looper[looper_idx * 2 + 1] * f;
            }
            else if (looper_mode == LOOPER_REC)
            {
                // Leave one bit for headroom
                looper[looper_idx * 2] = 16384 * input0;
                looper[looper_idx * 2 + 1] = 16384 * input1;
            }
            looper_idx++;
        }
        x[i * 2] = output0 + input0;
        x[i * 2 + 1] = output1 + input1;
    }
    if (halt_timeout_counter > frames)
        halt_timeout_counter -= frames;
    else
        halt_counter = halt_timeout_counter = 0;
}

// This will delegate the sequence restart to the processing
// hook as the tempo check will recognize a tempo change and reset the
// sequence. Resetting sequence from parameter changes proved to be
// unreliable; sometimes the looper began playing at wrong offset.
#define DELEGATE_SEQUENCE_RESTART_TO_MODFX_PROCESS \
    tempo = 0

void REVFX_PARAM(uint8_t index, int32_t value)
{
    const float v = q31_to_f32(value);
    if (index == k_user_revfx_param_time)
    {
        const uint8_t pattern_idx = (int)(N_PATTERNS * 0.999f * v);
        const uint8_t * new_p = patterns[pattern_idx];
        if (new_p != pattern)
        {
            DELEGATE_SEQUENCE_RESTART_TO_MODFX_PROCESS;
            blip_counter = SHORT_BLIP_LENGTH;
            blip_polarity_mask = new_p[STEP_DIV_IDX] == 4 ?
                BLIP_POLARITY_MASK_HI : BLIP_POLARITY_MASK_LO;
        }
        if (wait_thd_cross == WAIT_THD_CROSS_ACTIVE)
        {
            alt_drums = !alt_drums;
            set_drum_waveforms();
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
    else if (index == k_user_revfx_param_depth)
    {
        const uint8_t new_halt_status = v < 0.001f;
        if (halt_status && !new_halt_status) // Depth: 0 -> x
        {
            DELEGATE_SEQUENCE_RESTART_TO_MODFX_PROCESS;
            if (wait_thd_cross == WAIT_THD_CROSS_ARMED)
            {
                wait_thd_cross = WAIT_THD_CROSS_ACTIVE;
                blip_counter = 0;
                seq_trig_thd *= 2;
            }
        }
        else if (!halt_status && new_halt_status) // Depth x -> 0
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
        halt_status = new_halt_status;
        gain = v;
    }
    else if (index == k_user_revfx_param_shift_depth)
    {
        looper_vol = 2 * v;
    }
}
