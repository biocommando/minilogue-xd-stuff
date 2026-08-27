#include "../wav_handler/wav_handler.h"
#include "../src/basic_oscillator.h"
#include "../src/synth_random.h"

#include "synth_api.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

typedef struct
{
    int type;
    int value;
} SeqEvt;

#define NOTE_ON_EVT 1
#define NOTE_OFF_EVT 2
#define EMPTY_EVT 0

#define N_EVENTS 32

typedef struct
{
    char fn[1024];
    uint16_t user_p[6], shape, sshape;
    float pitch_lfo_amt, shape_lfo_amt;
    int step_sz;
    SeqEvt evt[N_EVENTS];
    int n_evt;
    float vol;
    int length_sec;
} Params;

static void parse_args(Params *p, int argc, char **argv)
{
    for (int i = 1; i < argc; i++)
    {
        char sel = argv[i][0];
        if (!sel)
            continue;
        if (!strcmp(argv[i], "--help"))
        {
            printf("Usage: %s [arguments...]\n", argv[0]);
            puts("File Output:");
            puts("  o<path>  Specify the output file name (default output.wav)\n");
            puts("Events:");
            puts("  +<num>   Add a NOTE_ON event with the specified value");
            puts("  -<num>   Add a NOTE_OFF event with the specified value");
            puts("  _<num>   Add <num> EMPTY events (pads the event list)\n");
            puts("Each argument configures one or more steps and increments");
            puts("the sequence length by the number of configured steps");
            puts("If no events are configured, uses a single event at the start of the file\n");
            puts("Configuration:");
            puts("  t<num>   Set tempo / step size based on BPM");
            puts("  s<num>   Set shape value");
            puts("  S<num>   Set shift shape value");
            puts("  p<num>   Set pitch LFO amount (value scaled by 0.01)");
            puts("  h<num>   Set shape LFO amount (value scaled by 0.01)");
            puts("  A..F<num> Set user parameter 1 through 6 to the specified value (no scaling)\n");
            printf("Note: The total number of events cannot exceed %d.\n", N_EVENTS);
            exit(0);
        }
        const char *arg = &argv[i][1];

        int v = 0;
        sscanf(arg, "%d", &v);
        if (sel == '+' || sel == '-' || sel == '_')
        {
            if (p->n_evt == N_EVENTS)
            {
                printf("Warning: only %d events supported\n", N_EVENTS);
            }
            else
            {
                if (sel == '+')
                {
                    p->evt[p->n_evt].type = NOTE_ON_EVT;
                    p->evt[p->n_evt].value = v;
                    p->n_evt++;
                }
                else if (sel == '-')
                {
                    p->evt[p->n_evt].type = NOTE_OFF_EVT;
                    p->evt[p->n_evt].value = v;
                    p->n_evt++;
                }
                else if (sel == '_')
                {
                    if (v <= 0)
                        v = 1;
                    v = p->n_evt + v;
                    if (v > N_EVENTS)
                    {
                        printf("Warning: only %d events supported\n", N_EVENTS);
                        v = N_EVENTS;
                    }
                    while (p->n_evt < v)
                    {
                        p->evt[p->n_evt].type = EMPTY_EVT;
                        p->n_evt++;
                    }
                }
            }
        }
        if (sel == 't')
        {
            p->step_sz = 60.0 / 4 / v * k_samplerate;
        }
        if (sel >= 'A' && sel <= 'F')
        {
            p->user_p[sel - 'A'] = v;
        }
        if (sel == 's')
        {
            p->shape = v;
        }
        if (sel == 'S')
        {
            p->sshape = v;
        }
        if (sel == 'p')
        {
            p->pitch_lfo_amt = 0.01 * v;
        }
        if (sel == 'h')
        {
            p->shape_lfo_amt = 0.01 * v;
        }
        if (sel == 'o')
        {
            strcpy(p->fn, arg);
        }
        if (sel == 'v')
        {
            p->vol = 0.01 * v;
        }
        if (sel == 'l')
        {
            p->length_sec = v;
        }
    }
    if (p->n_evt == 0 && p->n_evt < N_EVENTS)
    {
        p->evt[p->n_evt].type = NOTE_ON_EVT;
        p->evt[p->n_evt].value = 60;
        p->n_evt++;
    }
}

static void init_oscillator_module(Params *p)
{
    OSC_INIT(0, 0);
    for (int i = 0; i < 6; i++)
    {
        OSC_PARAM(i, p->user_p[i]);
    }
    OSC_PARAM(k_user_osc_param_shape, p->shape);
    OSC_PARAM(k_user_osc_param_shiftshape, p->sshape);
}

static uint16_t note_and_fine_to_pitch(int note, float finetune)
{
    int coarse = 0;
    int fine = 255 * finetune;
    if (fine < 0)
    {
        coarse -= 1;
        fine = 255 + fine;
    }
    return ((note + coarse) << 8) | (fine & 0xFF);
}

static void render_sound(Params *p)
{
    struct wav_file wav;
    int length_sec = p->length_sec ? p->length_sec * k_samplerate : p->step_sz * (N_EVENTS + 1);
    create_wav_file(&wav, length_sec, 1, 16, k_samplerate);
    init_oscillator_module(p);

    BasicOscillator lfo;
    init_BasicOscillator(&lfo, k_samplerate);
    BasicOscillator_setFrequency(&lfo, 4.0 / k_samplerate);

    int current_pitch = 0;
    int sample_i = 0;
    int bsize = 8;
    int evt_i = 0;
    int32_t q31_buf[64];
    while (sample_i < wav.num_frames)
    {
        float lfo_v = BasicOscillator_getValue(&lfo, OSC_TRIANGLE);

        user_osc_param_t params;

        params.pitch = note_and_fine_to_pitch(current_pitch, lfo_v * p->pitch_lfo_amt);
        params.shape_lfo = f32_to_q31(lfo_v * p->shape_lfo_amt);
        OSC_CYCLE(&params, q31_buf, bsize);
        for (int i = 0; i < bsize; i++)
        {
            if (sample_i == evt_i * p->step_sz && evt_i < p->n_evt)
            {
                int type = p->evt[evt_i].type;
                if (type == NOTE_ON_EVT)
                {
                    current_pitch = p->evt[evt_i].value;
                    params.pitch = note_and_fine_to_pitch(current_pitch, lfo_v * p->pitch_lfo_amt);
                    OSC_NOTEON(&params);
                }
                if (type == NOTE_OFF_EVT)
                {
                    OSC_NOTEOFF(&params);
                }
                evt_i++;
            }
            BasicOscillator_calculateNext(&lfo);
            float f = q31_to_f32(q31_buf[i]) * p->vol;
            wav_set_normalized(&wav, sample_i, &f);
            sample_i++;
        }

        bsize *= 2;
        if (bsize > 64)
            bsize = 8;
    }
    if (write_wav_file(p->fn, &wav) != 0)
        puts("Writing output file failed");
    free_wav_file(&wav);
}

int main(int argc, char **argv)
{
    Params p;
    memset(&p, 0, sizeof(p));
    strcpy(p.fn, "output.wav");
    p.step_sz = 60.0 / 4 / 120 * k_samplerate;
    p.vol = 1;
    parse_args(&p, argc, argv);
    render_sound(&p);
    return 0;
}
