#include "bl_oscillator.h"
#include "userosc.h"
#include "manifest_params.h"
#include "stuff_util.h"
#ifdef SOUNDTOOL
#include <stdio.h>
#endif

struct step {
    uint8_t note;
    uint32_t delta;
};

#define N_STEPS 2048
static struct step steps[N_STEPS];
static uint32_t n_steps = 0;

static struct {
    uint32_t step_idx, state, inc, pos, next_event_pos;
} seq_state;

#define sqst_init 0
#define sqst_stop 1
#define sqst_program 2
#define sqst_play 3

/*
 * Sequence writing state machine:
 * 0 -> 127 -> 0 ... = reset sequence
 * each 0/127 write increment counter by 1
 *   --> at counter value 1: set state to "note"
 *   --> at counter value 3: set state to "init" (prevent writing new
 *                           notes accidentally)
 *   --> at counter value 5: reset number of steps in sequence and set
 *                           state to "note"
 * So recommended practices:
 * - start sequence transmission with 0 127 0 127 0 127
 * - after note transmission send 0
 * - after sequence transmission send 127 0 (so note end + seq end =
 *   0 127 0 which transitions state to "init")
 * 
 * in state "note":
 * increment number of steps (up to N_STEPS)
 * 1...126 set current step midi note number to this and set state to "delta"
 *         and set current step delta to 0.
 * 
 * in state "delta":
 * 1...63  set current step delta (D) in samples in the following manner:
 *         D = (D << 5) | (CC_VALUE >> 1)
 *         if D wraps over (32 bit uint, wraps at around 24 h), keep the
 *         old value and reset state to "init"
 *     126 acts as a "don't care" value. Send this if the next value should
 *         be the same as the previous value
 */
#define st_init 0
#define st_note 1
#define st_note_off_note 2
#define st_delta 3

static void handle_cc(uint8_t v)
{
    static uint8_t st = st_init, reset_seq_last_val = 0, reset_seq_cntr = 0;
    if (v == 0 || v == 127)
    {
        if (reset_seq_last_val != v || reset_seq_cntr == 0)
        {
            if (reset_seq_cntr < 255)
                reset_seq_cntr++;
            if (reset_seq_cntr == 1)
                st = st_note;
            if (reset_seq_cntr == 3)
            {
                seq_state.state = sqst_stop;
                st = st_init;
            }
            if (reset_seq_cntr == 5)
            {
                seq_state.state = sqst_program;
                n_steps = 0;
                st = st_note;
                seq_state.step_idx = 0;
                seq_state.pos = 0;
                seq_state.next_event_pos = 0;
            }
            reset_seq_last_val = v;
        }
        return;
    }
    else if (st == st_note)
    {
        reset_seq_cntr = 0;
        if (n_steps < N_STEPS)
        {
            n_steps++;
        }
        if (n_steps <= N_STEPS)
        {
            steps[n_steps - 1].delta = 0;
            steps[n_steps - 1].note = v;
        }
        st = v == 1 ? st_note_off_note : st_delta;
    }
    else if (st == st_note_off_note)
    {
        reset_seq_cntr = 0;
        if (n_steps <= N_STEPS)
        {
            steps[n_steps - 1].note = v | 0x80;
        }
        st = st_delta;
    }
    else if (st == st_delta)
    {
        reset_seq_cntr = 0;
        if (n_steps <= N_STEPS && v != 126)
        {
            uint32_t delta = steps[n_steps - 1].delta;
            const uint32_t low_word = (v >> 1) & 0x1f;
            const uint32_t high_words = delta << 5;
            delta = high_words | low_word;
            steps[n_steps - 1].delta = delta;
            if (delta & 0xf8000000)
                st = st_init;
        }
    }
}

// For fixed sequence mode
static const float note_list[] = {0.0248031, 0.026278, 0.0278406, 0.0294961, 0.03125, 0.0331082, 0.0350769, 0.0371627, 0.0393725, 0.0417137, 0.0441942, 0.0468221, 0.0496063, 0.052556, 0.0556812, 
0.0589921, 0.0625, 0.0662164, 0.0701539, 0.0743254, 0.0787451, 0.0834275, 0.0883883, 0.0936442, 0.0992126, 0.105112, 0.111362, 0.117984, 0.125, 0.132433, 0.140308, 
0.148651, 0.15749, 0.166855, 0.176777, 0.187288, 0.198425, 0.210224, 0.222725, 0.235969, 0.25, 0.264866, 0.280616, 0.297302, 0.31498, 0.33371, 0.353553, 
0.374577, 0.39685, 0.420448, 0.445449, 0.471937, 0.5, 0.529732, 0.561231, 0.594604, 0.629961, 0.66742, 0.707107, 0.749154, 0.793701, 0.840896, 0.890899, 
0.943874, 1.0, 1.05946, 1.12246, 1.18921, 1.25992, 1.33484, 1.41421, 1.49831, 1.5874, 1.68179, 1.7818, 1.88775, 2.0, 2.11893, 2.24492, 
2.37841, 2.51984, 2.66968, 2.82843, 2.99661, 3.1748, 3.36359, 3.56359, 3.7755, 4.0, 4.23785, 4.48985, 4.75683, 5.03968, 5.33936, 5.65685, 
5.99323, 6.3496, 6.72717, 7.12719, 7.55099, 8.0, 8.4757, 8.9797, 9.51366, 10.0794, 10.6787, 11.3137, 11.9865, 12.6992, 13.4543, 14.2544, 
15.102, 16.0, 16.9514, 17.9594, 19.0273, 20.1587, 21.3574, 22.6274, 23.9729, 25.3984, 26.9087, 28.5088, 30.204, 32.0, 33.9028, 35.9188, 
38.0546,};
#define BASE_NOTE_HZ 329.6275569128699f
#define BASE_NOTE_INC (BASE_NOTE_HZ / k_samplerate)

// For keyboard tracking mode
static float note_factors[128] =
{
    1.0, 1.05946, 1.12246, 1.18921, 1.25992, 1.33484, 1.41421, 1.49831, 
    1.5874, 1.68179, 1.7818, 1.88775, 2.0, 2.11893, 2.24492, 2.37841, 2.51984, 
    2.66968, 2.82843, 2.99661, 3.1748, 3.36359, 3.56359, 3.7755, 4.0, 4.23785, 
    4.48985, 4.75683, 5.03968, 5.33936, 5.65685, 5.99323, 6.3496, 6.72717, 
    7.12719, 7.55099, 8.0, 8.4757, 8.9797, 9.51366, 10.0794, 10.6787, 11.3137, 
    11.9865, 12.6992, 13.4543, 14.2544, 15.102, 16.0, 16.9514, 17.9594, 
    19.0273, 20.1587, 21.3574, 22.6274, 23.9729, 25.3984, 26.9087, 28.5088, 
    30.204, 32.0, 33.9028, 35.9188, 38.0546, 40.3175, 42.7149, 45.2548, 
    47.9458, 50.7968, 53.8174, 57.0175, 60.408, 64.0, 67.8056, 71.8376, 
    76.1093, 80.6349, 85.4298, 90.5097, 95.8917, 101.594, 107.635, 114.035, 
    120.816, 128.0, 135.611, 143.675, 152.219, 161.27, 170.86, 181.019, 
    191.783, 203.187, 215.269, 228.07, 241.632, 256.0, 271.223, 287.35, 
    304.437, 322.54, 341.719, 362.039, 383.567, 406.375, 430.539, 456.14, 
    483.264, 512.0, 542.445, 574.701, 608.874, 645.08, 683.438, 724.077, 
    767.133, 812.749, 861.078, 912.28, 966.527, 1024.0, 1084.89, 1149.4, 
    1217.75, 1290.16, 1366.88, 1448.15, 1534.27, 
};
static float note_factors_inv[128] =
{
    1.0, 0.943874, 0.890899, 0.840896, 0.793701, 0.749154, 0.707107, 0.66742, 
    0.629961, 0.594604, 0.561231, 0.529732, 0.5, 0.471937, 0.445449, 0.420448, 
    0.39685, 0.374577, 0.353553, 0.33371, 0.31498, 0.297302, 0.280616, 
    0.264866, 0.25, 0.235969, 0.222725, 0.210224, 0.198425, 0.187288, 0.176777, 
    0.166855, 0.15749, 0.148651, 0.140308, 0.132433, 0.125, 0.117984, 0.111362, 
    0.105112, 0.0992126, 0.0936442, 0.0883883, 0.0834275, 0.0787451, 0.0743254, 
    0.0701539, 0.0662164, 0.0625, 0.0589921, 0.0556812, 0.052556, 0.0496063, 
    0.0468221, 0.0441942, 0.0417137, 0.0393725, 0.0371627, 0.0350769, 0.0331082, 
    0.03125, 0.0294961, 0.0278406, 0.026278, 0.0248031, 0.023411, 0.0220971, 
    0.0208569, 0.0196863, 0.0185814, 0.0175385, 0.0165541, 0.015625, 0.014748, 
    0.0139203, 0.013139, 0.0124016, 0.0117055, 0.0110485, 0.0104284, 0.00984313, 
    0.00929068, 0.00876923, 0.00827706, 0.0078125, 0.00737402, 0.00696015, 
    0.0065695, 0.00620079, 0.00585276, 0.00552427, 0.00521422, 0.00492157, 
    0.00464534, 0.00438462, 0.00413853, 0.00390625, 0.00368701, 0.00348007, 
    0.00328475, 0.00310039, 0.00292638, 0.00276214, 0.00260711, 0.00246078, 
    0.00232267, 0.00219231, 0.00206926, 0.00195312, 0.0018435, 0.00174004, 
    0.00164238, 0.0015502, 0.00146319, 0.00138107, 0.00130355, 0.00123039, 
    0.00116134, 0.00109615, 0.00103463, 0.000976562, 0.000921752, 0.000870018, 
    0.000821188, 0.000775098, 0.000731595, 0.000690534, 0.000651777, 
};

#define NOTE_FREE_THD 0.01
#define NOTE_DEAD_THD 0.0001

typedef struct {
    BlOscillator osc;
    uint8_t note, note_free;
    enum OscType waveform;
    float vol;
    float env;
} Voice;

#define N_VOICES 4

static Voice voices[N_VOICES];

#define FOR_EACH_VOICE(voice_var, expr) \
    for (Voice *voice_var = voices; voice_var < voices + N_VOICES; voice_var++) expr

static uint8_t kb_tracking = 0, split_at_note = 0, split_mode = 0, current_note,
               steal_idx = 0, kb_track_base_note;
static float current_inc = BASE_NOTE_INC, env_param = 0, volume = 1.0f / N_VOICES;

static inline void update_current_inc(const user_osc_param_t *const params)
{
    if (kb_tracking)
        current_inc = osc_w0f_for_note((params->pitch) >> 8, params->pitch & 0xFF);
    else
        current_inc = BASE_NOTE_INC;
    current_note = (params->pitch) >> 8;
}


static inline void update_inc(Voice *v)
{
    int16_t note = v->note;
    float inc = current_inc;
    if (kb_tracking)
    {
        note -= kb_track_base_note;
        if (note > 0)
            inc *= note_factors[note];
        else
            inc *= note_factors_inv[-note];
        note += current_note;
    }
    else
        inc *= note_list[v->note];
    BlOscillator_setFrequency(&v->osc, inc, note);
}

static inline void progress_sequence()
{
    if (!n_steps || seq_state.state != sqst_play) return;
    
    seq_state.pos += seq_state.inc;
    uint8_t max_iter = n_steps > N_VOICES * 2 ? N_VOICES * 2 : n_steps;
    while (seq_state.pos >= seq_state.next_event_pos && max_iter--)
    {
        const uint8_t note = steps[seq_state.step_idx].note;
        const uint32_t delta = steps[seq_state.step_idx].delta;
#ifdef SOUNDTOOL
        printf("%u : %u %s %u\n", seq_state.pos, note & 0x7f, (note & 0x80) ? "off" : "on", delta);
#endif
        seq_state.pos = 0;
        seq_state.next_event_pos = delta;

        if (note & 0x80)
        {
            const uint8_t note_wo_flag = note & 0x7f;
            FOR_EACH_VOICE(v, {
                if (v->note == note_wo_flag)
                    v->env = env_param;
            });
        }
        else
        {
            Voice *voice = NULL;
            FOR_EACH_VOICE(v, {
                if (v->note_free) {
                    voice = v;
                }
            });
            if (!voice) {
                voice = &voices[steal_idx];
                steal_idx = (steal_idx + 1) % N_VOICES;
            }

            voice->note = note;
            voice->vol = NOTE_FREE_THD;
            voice->env = 1.14;
            voice->note_free = 0;
            update_inc(voice);
        }
        seq_state.step_idx++;
        if (seq_state.step_idx >= n_steps)
            seq_state.step_idx = 0;
    }
    
}

static inline float process_voice(Voice *v)
{
    if (v->note == 0)
        return 0;
    BlOscillator_calculateNext(&v->osc);
    float out = BlOscillator_getValue(&v->osc, v->waveform);
    out *= v->vol;
    v->vol *= v->env;
    if (v->vol < NOTE_FREE_THD)
    {
        v->note_free = 1;
        if (v->vol < NOTE_DEAD_THD)
            v->note = 0;
    }
    else if (v->vol > 1)
    {
        v->vol = 1;
        if (v->env > 1)
            v->env = 1;
    }
    return out;
}

#ifdef SOUNDTOOL
void read_cc_msg_from_file()
{
    FILE *f = fopen("midimsg.txt", "r");
    if (!f)
    {
        puts("Cannot open midimsg.txt");
        return;
    }
    int ii = 0;
    while (!feof(f))
    {
        char buf[100];
        const char *r = fgets(buf, sizeof(buf), f);
        if (!r || *buf == '#') break;
        uint32_t cc;
        if (sscanf(buf, "%u", &cc));
            OSC_PARAM(k_user_osc_param_shiftshape, cc << 3);
    }
    fclose(f);
    puts("*** Sequencer steps ***");
    printf("Count = %u\n", n_steps);
    for (unsigned n = 0; n < n_steps; n++)
    {
        printf("   %3u: %3u, +%u\n", n, steps[n].note, steps[n].delta);
    }
    puts("***********************");  
}  
#endif

void OSC_INIT(uint32_t platform, uint32_t api)
{
    (void) platform;
    (void) api;
    float phase = 0;
    FOR_EACH_VOICE(v, {
        v->osc.phase = phase;
        phase = ((int)(phase * 39798535) ^ 356295141) / 205884.27f;
        phase -= (int)phase;
        v->osc.frequency = phase * 0.01f;
        v->note = 0;
        v->note_free = 1;
    });
    seq_state.state = sqst_init;
    seq_state.inc = 1;
#ifdef SOUNDTOOL
    read_cc_msg_from_file();
#endif
}

void OSC_CYCLE(const user_osc_param_t *const params, int32_t *yn, const uint32_t frames)
{
    update_current_inc(params);
    FOR_EACH_VOICE(v, update_inc(v));

    OSC_LOOP(y, yn, frames)
    {
        progress_sequence();
        float out = 0;
        FOR_EACH_VOICE(v, out += process_voice(v));
        out *= volume;
        *(y++) = safe_f32_to_q31(out);
    }
}

void OSC_NOTEON(const user_osc_param_t *const params)
{
    if (seq_state.state == sqst_program)
        return;
    const uint8_t note = (params->pitch) >> 8;
    FOR_EACH_VOICE(v, {
        v->note = 0;
        v->note_free = 1;
    });
    if (split_mode == 0 && note < split_at_note)
        return;
    if (split_mode == 1 && note > split_at_note)
        return;
    seq_state.pos = 0;
    seq_state.next_event_pos = 0;
    seq_state.step_idx = 0;
    seq_state.state = sqst_play;
    kb_track_base_note = steps[0].note;
}

void OSC_NOTEOFF(const user_osc_param_t *const params)
{
    (void) params;
    seq_state.state = sqst_stop;
}

void OSC_PARAM(uint16_t index, uint16_t value)
{
#ifdef SOUNDTOOL
static uint16_t shape_prev = 0xffff, sshape_prev = 0xffff;
if (index == k_user_osc_param_shape)
{
    if (shape_prev == value) return;
    shape_prev = value;
}
if (index == k_user_osc_param_shiftshape)
{
    if (sshape_prev == value) return;
    sshape_prev = value;
}
#endif
    switch (index)
    {
        case USER_PARAM__Waveform__idx:
            {
                enum OscType t;
                if (value == 0) t = OSC_SAW;
                else if (value == 1) t = OSC_SQUARE;
                else t = OSC_TRIANGLE;
                FOR_EACH_VOICE(v, v->waveform = t);
            }
            break;
        case USER_PARAM__Decay__idx:
            {
                env_param = 0.99999f - 0.001f * (1 - value / 100.0f);
            }
            break;
        case USER_PARAM__Split__idx:
            {
                if (value < 100) {
                    split_at_note = value;
                    split_mode = 0;
                } else  {
                    split_at_note = value - 100;
                    split_mode = 1;
                }
            }
            break;
        case USER_PARAM__Tracking__idx:
            {
                kb_tracking = value ? 0 : 1;
            }
            break;
        case k_user_osc_param_shape:
            {
                const uint8_t shift = value >> 7;
                seq_state.inc = 1 << shift;
            }
            break;
        case k_user_osc_param_shiftshape:
            handle_cc((value >> 3) & 0x7f);
            break;
        default:
            break;
    }
}
