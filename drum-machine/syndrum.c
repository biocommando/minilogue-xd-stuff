#include "syndrum.h"
#ifndef GENWAVES
#include "userosc.h"
#else
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "wav_handler/wav_handler.h"
#define k_samplerate 48000
#endif
#include "simple_oscillator.h"
#include "flt.h"

static SimpleOscillator syn_drum_osc = {.phase = 0,.frequency = 0.0001 };
static SimpleOscillator syn_drum_lfo = {.phase = 0,.frequency = 0.0001 };

static float syn_drum_pitch_mod;
static float syn_drum_vol, syn_drum_vol_mod;
static float syn_drum_osc_mix, syn_drum_noise_mix;
static enum OscType syn_drum_osc_type;

static uint8_t use_hpf, use_lfo;
// Fixed high pass filter at 6 kHz
static struct filter_state hpf = {.factor = 0.5600991537930968,.state0 = 0,.state1 = 0 };

// Fast pseudo random implementation
static uint32_t rand_state = 123;
static uint32_t next_random()
{
    rand_state ^= rand_state << 13;
    rand_state ^= rand_state >> 17;
    rand_state ^= rand_state << 5;
    return rand_state;
}

static float noise;             // last noise value (-1..1) saved here to prevent the need for rng calls elsewhere
static uint8_t variation = 0;
float process_syn_drum()
{
    SimpleOscillator_calculateNext(&syn_drum_osc);
    float v = SimpleOscillator_getValue(&syn_drum_osc, syn_drum_osc_type);
    v *= syn_drum_osc_mix;
    noise = (0x7fffffff - (int) next_random()) / (float) (0x7fffffff);
    v += noise * syn_drum_noise_mix;
    if (use_hpf)
        v = process_hp_filter(&hpf, v);
    if (use_lfo)
    {
        SimpleOscillator_calculateNext(&syn_drum_lfo);
        v *= 1 - syn_drum_lfo.phase;
    }
    v *= syn_drum_vol;
    syn_drum_vol *= syn_drum_vol_mod;
    syn_drum_osc.frequency *= syn_drum_pitch_mod;
    return v;
}

#define SD_USE_HPF 1
#define SD_HPF_LOW_FREQ 2
#define SD_USE_LFO 4
#define SD_USE_LFO_FAST 8
#define SD_LONG_ENV 16

#define SD_FREQ(x) ((x) & 0x1f)
#define SD_WAVEFORM(x) ((enum OscType)(((x)>>6) & 3))

#define SD_PACK_FREQ_WF(waveform, frequency) ((((uint8_t)(waveform)) << 6) | (frequency))

struct syn_drum_params_t
{
    uint8_t freq_wf;
    uint8_t pitch_mod;
    uint8_t osc_mix;
    uint8_t noise_mix;
    uint8_t vol_mod;
    uint8_t flags;
};

static const struct syn_drum_params_t syn_drum_params[] = {
    // kick 1
    {
     .freq_wf = SD_PACK_FREQ_WF(OSC_TRIANGLE, 8),
     .pitch_mod = 140,
     .osc_mix = 255,
     .noise_mix = 0,
     .vol_mod = 150,
     .flags = 0,
     },

    // kick 2
    {
     .freq_wf = SD_PACK_FREQ_WF(OSC_TRIANGLE, 10),
     .pitch_mod = 180,
     .osc_mix = 255,
     .noise_mix = 0,
     .vol_mod = 150,
     .flags = 0,
     },

    // snare 1
    {
     .freq_wf = 0,
     .pitch_mod = 0,
     .osc_mix = 0,
     .noise_mix = 255,
     .vol_mod = 70,
     .flags = 0,
     },

    // snare 2
    {
     .freq_wf = SD_PACK_FREQ_WF(OSC_SQUARE, 5),
     .pitch_mod = 200,
     .osc_mix = 32,
     .noise_mix = 255,
     .vol_mod = 100,
     .flags = 0,
     },

    // hcp
    {
     .freq_wf = SD_PACK_FREQ_WF(OSC_SQUARE, 20),
     .pitch_mod = 80,
     .osc_mix = 50,
     .noise_mix = 255,
     .vol_mod = 120,
     .flags = SD_USE_LFO,
     },

    // hhc
    {
     .freq_wf = 0,
     .pitch_mod = 0,
     .osc_mix = 0,
     .noise_mix = 255,
     .vol_mod = 0,
     .flags = SD_USE_HPF,
     },

    // tam
    {
     .freq_wf = SD_PACK_FREQ_WF(OSC_SAW, 31),
     .pitch_mod = 252,
     .osc_mix = 255,
     .noise_mix = 128,
     .vol_mod = 0,
     .flags = SD_USE_LFO | SD_USE_HPF,
     },

    // hho
    {
     .freq_wf = 0,
     .pitch_mod = 0,
     .osc_mix = 0,
     .noise_mix = 255,
     .vol_mod = 160,
     .flags = SD_USE_HPF,
     },

    // cow
    {
     .freq_wf = SD_PACK_FREQ_WF(OSC_SQUARE, 20),
     .pitch_mod = 255,
     .osc_mix = 180,
     .noise_mix = 0,
     .vol_mod = 10,
     .flags = 0,
     },

    // crs
    {
     .freq_wf = 0,
     .pitch_mod = 0,
     .osc_mix = 0,
     .noise_mix = 255,
     .vol_mod = 210,
     .flags = SD_HPF_LOW_FREQ,
     },

    // rim
    {
     .freq_wf = SD_PACK_FREQ_WF(OSC_SQUARE, 1),
     .pitch_mod = 0,
     .osc_mix = 128,
     .noise_mix = 128,
     .vol_mod = 0,
     .flags = 0,
     },

    // ht
    {
     .freq_wf = SD_PACK_FREQ_WF(OSC_TRIANGLE, 25),
     .pitch_mod = 150,
     .osc_mix = 192,
     .noise_mix = 20,
     .vol_mod = 140,
     .flags = 0,
     },

};

#define ENV_MAX_LEN 0.9995
#define EXP_ENV(x) ENV_MAX_LEN + (1 - ENV_MAX_LEN) * x
#define uint8_to_f(x) (x / 255.0)

static inline void _set_syn_drum_params(const struct syn_drum_params_t *params)
{
    const uint8_t flags = params->flags ^ variation;
    syn_drum_osc_type = SD_WAVEFORM(params->freq_wf);
    syn_drum_osc.frequency = (SD_FREQ(params->freq_wf) * 32) / (float) k_samplerate;
    syn_drum_osc.phase = noise < 0 ? -noise : noise;
    syn_drum_pitch_mod = EXP_ENV(uint8_to_f(params->pitch_mod));
    syn_drum_osc_mix = uint8_to_f(params->osc_mix);
    syn_drum_noise_mix = uint8_to_f(params->noise_mix);
    syn_drum_vol_mod = EXP_ENV(uint8_to_f(params->vol_mod));
    syn_drum_vol = 1;
    use_hpf = flags & (SD_USE_HPF | SD_HPF_LOW_FREQ);
    if (flags & (SD_USE_HPF | SD_HPF_LOW_FREQ))
        hpf.factor = 0.7180302001078813;        // 3 kHz
    else if (flags & SD_HPF_LOW_FREQ)
        hpf.factor = 0.8842517205783795;        // 1 kHz
    else
        hpf.factor = 0.5600991537930968;        // 6 kHz
    use_lfo = flags & (SD_USE_LFO | SD_USE_LFO_FAST);

    int lfo_freq = 12;
    if (flags & SD_USE_LFO)
        lfo_freq += 13;
    if (flags & SD_USE_LFO_FAST)
        lfo_freq += 40;
    syn_drum_lfo.frequency = lfo_freq / (float) k_samplerate;

    if (flags & SD_LONG_ENV)
    {
        syn_drum_vol_mod = syn_drum_vol_mod + (1 - syn_drum_vol_mod) * 0.5;
        syn_drum_pitch_mod = syn_drum_vol_mod + (1 - syn_drum_vol_mod) * 0.5;
    }

    syn_drum_lfo.phase = 0;
}

void set_syn_drum_params(int drum_idx, float freq_modifier)
{
    _set_syn_drum_params(&syn_drum_params[drum_idx]);
    syn_drum_osc.frequency *= freq_modifier;
}

void set_syn_drum_variation(uint8_t new_variation)
{
    variation = new_variation;
}

#ifdef GENWAVES

struct syn_drum_params_t params;
int freq = 0, waveform = (int) OSC_SAW;

void read_params(FILE *f, const char *section)
{
    memset(&params, 0, sizeof(params));
    int sect_found = section ? 0 : 1;
    while (!feof(f))
    {
        char buf[100], cmd[100], val[100];
        fgets(buf, sizeof(buf), f);
        if (buf[0] == '#' || buf[0] == 0)
            continue;
        if (sscanf(buf, "%s = %s", cmd, val) != 2)
            continue;

        if (!strcmp("section", cmd))
        {
            if (section)
            {
                if (!strcmp(section, val))
                {
                    sect_found++;
                    if (sect_found > 1)
                        break;
                }
            }
            continue;
        }
        if (!sect_found)
            continue;

        if (!strcmp("freq", cmd))
            freq = atoi(val);
        else if (!strcmp("wf", cmd))
        {
            if (!strcmp("OSC_TRIANGLE", val))
                waveform = (int) OSC_TRIANGLE;
            if (!strcmp("OSC_SQUARE", val))
                waveform = (int) OSC_SQUARE;
        }
        else if (!strcmp("pitch_mod", cmd))
            params.pitch_mod = atoi(val);
        else if (!strcmp("osc_mix", cmd))
            params.osc_mix = atoi(val);
        else if (!strcmp("noise_mix", cmd))
            params.noise_mix = atoi(val);
        else if (!strcmp("vol_mod", cmd))
            params.vol_mod = atoi(val);
        else if (!strcmp("flags", cmd) && !strcmp("SD_USE_HPF", val))
            params.flags |= SD_USE_HPF;
        else if (!strcmp("flags", cmd) && !strcmp("SD_HPF_LOW_FREQ", val))
            params.flags |= SD_HPF_LOW_FREQ;
        else if (!strcmp("flags", cmd) && !strcmp("SD_USE_LFO", val))
            params.flags |= SD_USE_LFO;
        else if (!strcmp("flags", cmd) && !strcmp("SD_USE_LFO_FAST", val))
            params.flags |= SD_USE_LFO_FAST;
        else if (!strcmp("flags", cmd) && !strcmp("SD_LONG_ENV", val))
            params.flags |= SD_LONG_ENV;
        else if (!strcmp("variation", cmd))
        {
            variation = atoi(val);
        }
        else
            printf("Could not parse config line '%s'\n", buf);
    }
    params.freq_wf = SD_PACK_FREQ_WF(waveform, freq);
}

void params_to_code()
{
    printf("{\n");
    if (params.freq_wf == 0)
        printf(".freq_wf = 0, // freq\n");
    else
        printf(".freq_wf = SD_PACK_FREQ_WF(%s, %d)\n", waveform == (int) OSC_TRIANGLE
               ? "OSC_TRIANGLE" : (waveform == (int) OSC_SQUARE ? "OSC_SQUARE" : "OSC_SAW"), freq);
    printf(".pitch_mod = %u, // pitch mod\n", params.pitch_mod);
    printf(".osc_mix = %u, // osc mix\n", params.osc_mix);
    printf(".noise_mix = %u, // noise mix\n", params.noise_mix);
    printf(".vol_mod = %u, // vol mod\n", params.vol_mod);
    printf(".flags = 0");
    if (params.flags & SD_USE_HPF)
        printf(" | SD_USE_HPF");
    if (params.flags & SD_HPF_LOW_FREQ)
        printf(" | SD_HPF_LOW_FREQ");
    if (params.flags & SD_USE_LFO)
        printf(" | SD_USE_LFO");
    if (params.flags & SD_USE_LFO_FAST)
        printf(" | SD_USE_LFO_FAST");
    if (params.flags & SD_LONG_ENV)
        printf(" | SD_LONG_ENV");
    printf(" // flags\n}\n");
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        puts("Required parameters: input path, output path");
        return 1;
    }
    FILE *f = fopen(argv[1], "r");
    if (!f)
    {
        puts("Error opening output file");
        return 1;
    }
    read_params(f, argc >= 4 ? argv[3] : NULL);
    fclose(f);
    struct wav_file wav;
    create_wav_file(&wav, k_samplerate * 4, 1, 16, k_samplerate);
    _set_syn_drum_params(&params);
    for (int i = 0; i < k_samplerate * 4; i++)
    {
        float v = process_syn_drum();
        wav_set_normalized(&wav, i, &v);
    }
    write_wav_file(argv[2], &wav);
    free_wav_file(&wav);
    params_to_code();
    return 0;
}
#endif
