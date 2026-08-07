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

int main(int argc, char **argv)
{
    char fn[128] = "output.wav";

    SeqEvt evt[32];
    int n_evt = 0;

    uint16_t user_p[6] = {0, 0, 0, 0, 0, 0}, shape = 0, sshape = 0;
    float pitch_lfo_amt = 0, shape_lfo_amt = 0;
    int step_sz = 60.0 / 4 / 120 * k_samplerate;
    for (int i = 1; i < argc; i++)
    {
        char sel = argv[i][0];
        if (!sel)
            continue;
        const char *arg = &argv[i][1];

        int v = 0;
        sscanf(arg, "%d", &v);
        if (sel == '+' || sel == '-' || sel == '_')
        {
            if (n_evt == 32)
            {
                puts("Warning: only 32 events supported");
            }
            else
            {
                if (sel == '+')
                {
                    evt[n_evt].type = NOTE_ON_EVT;
                    evt[n_evt].value = v;
                    n_evt++;
                }
                else if (sel == '-')
                {
                    evt[n_evt].type = NOTE_OFF_EVT;
                    evt[n_evt].value = v;
                    n_evt++;
                }
                else if (sel == '_')
                {
                    if (v <= 0)
                        v = 1;
                    v = n_evt + v;
                    if (v > 32)
                    {
                        puts("Warning: only 32 events supported");
                        v = 32;
                    }
                    while (n_evt < v)
                    {
                        evt[n_evt].type = EMPTY_EVT;
                        n_evt++;
                    }
                }
            }
        }
        if (sel == 't')
        {
            step_sz = 60.0 / 4 / v * k_samplerate;
        }
        if (sel >= 'A' && sel <= 'F')
        {
            user_p[sel - 'A'] = v;
        }
        if (sel == 's')
        {
            shape = v;
        }
        if (sel == 'S')
        {
            sshape = v;
        }
        if (sel == 'p')
        {
            pitch_lfo_amt = 0.01 * v;
        }
        if (sel == 'h')
        {
            shape_lfo_amt = 0.01 * v;
        }
        if (sel == 'o')
        {
            strcpy(fn, arg);
        }
    }
    if (n_evt == 0 && n_evt < 32)
    {
        evt[n_evt].type = NOTE_ON_EVT;
        evt[n_evt].value = 60;
        n_evt++;
    }
    struct wav_file wav;
    create_wav_file(&wav, step_sz * 33, 1, 16, k_samplerate);
    for (int i = 0; i < 6; i++)
    {
        OSC_PARAM(i, user_p[i]);
    }
    OSC_PARAM(k_user_osc_param_shape, shape);
    OSC_PARAM(k_user_osc_param_shiftshape, sshape);

    BasicOscillator lfo;
    init_BasicOscillator(&lfo, k_samplerate);
    BasicOscillator_setFrequency(&lfo, 4.0 / k_samplerate);

    int voice_state = 0, current_pitch = 0;
    int sample_i = 0;
    int bsize = 8;
    int evt_i = 0;
    int32_t q31_buf[64];
    while (sample_i < wav.num_frames)
    {
        float lfo_v = BasicOscillator_getValue(&lfo, OSC_TRIANGLE);

        int coarse = 0;
        int fine = 255 * lfo_v * pitch_lfo_amt;
        if (fine < 0)
        {
            coarse -= 1;
            fine = 255 + fine;
        }
        user_osc_param_t params;

        params.pitch = ((current_pitch + coarse) << 8) | (fine & 0xFF);
        params.shape_lfo = f32_to_q31(lfo_v * shape_lfo_amt);
        OSC_CYCLE(&params, q31_buf, bsize);
        for (int i = 0; i < bsize; i++)
        {
            if (sample_i == evt_i * step_sz && evt_i < n_evt)
            {
                int type = evt[evt_i].type;
                if (type == NOTE_ON_EVT)
                {
                    voice_state = 1;
                    current_pitch = evt[evt_i].value;
                    params.pitch = ((current_pitch + coarse) << 8) | fine & 0xFF;
                    OSC_NOTEON(&params);
                }
                if (type == NOTE_OFF_EVT)
                {
                    voice_state = 0;
                    OSC_NOTEOFF(&params);
                }
                evt_i++;
            }
            BasicOscillator_calculateNext(&lfo);
            float f = q31_to_f32(q31_buf[i]);
            wav_set_normalized(&wav, sample_i, &f);
            sample_i++;
        }

        bsize *= 2;
        if (bsize > 64)
            bsize = 8;
    }
    if (write_wav_file(fn, &wav) != 0)
        puts("Writing output file failed");
    free_wav_file(&wav);

    return 1;
}