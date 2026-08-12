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

#define SD_FREQ_WF_IDX 0
#define SD_PITCH_MOD_IDX 1
#define SD_OSC_MIX_IDX 2
#define SD_NOISE_MIX_IDX 3
#define SD_VOL_MOD_IDX 4
#define SD_FLAGS_IDX 5

#define SD_USE_HPF 1
#define SD_HPF_LOW_FREQ 2
#define SD_USE_LFO 4
#define SD_USE_LFO_FAST 8
#define SD_LONG_ENV 16

#define SD_FREQ(x) ((x) & 0x1f)
#define SD_WAVEFORM(x) ((enum OscType)(((x)>>6) & 3))

#define SD_PACK_FREQ_WF(waveform, frequency) ((((uint8_t)(waveform)) << 6) | (frequency))

static const uint8_t syn_drum_params[] = {
    // kick 1
    SD_PACK_FREQ_WF(OSC_TRIANGLE, 8),   // freq
    140,                        // pitch mod
    255,                        // osc mix
    0,                          // noise mix
    150,                        // vol mod
    0,                          // Flags
    // kick 2
    SD_PACK_FREQ_WF(OSC_TRIANGLE, 10),  // freq
    180,                        // pitch mod
    255,                        // osc mix
    0,                          // noise mix
    150,                        // vol mod
    0,                          // Flags
    // snare 1
    0,                          // freq
    0,                          // pitch mod
    0,                          // osc mix
    255,                        // noise mix
    70,                         // vol mod
    0,                          // Flags
    // snare 2
    SD_PACK_FREQ_WF(OSC_SQUARE, 5),     // freq
    200,                        // pitch mod
    32,                         // osc mix
    255,                        // noise mix
    100,                        // vol mod
    0,                          // Flags
    // hcp
    SD_PACK_FREQ_WF(OSC_SQUARE, 20),    // freq
    80,                         // pitch mod
    50,                         // osc mix
    255,                        // noise mix
    120,                        // vol mod
    SD_USE_LFO | SD_USE_HPF | SD_HPF_LOW_FREQ,  // Flags
    // hhc
    0,                          // freq
    0,                          // pitch mod
    0,                          // osc mix
    255,                        // noise mix
    0,                          // vol mod
    SD_USE_HPF,                 // Flags
    // tam
    SD_PACK_FREQ_WF(OSC_SAW, 31),       // freq
    252,                        // pitch mod
    255,                        // osc mix
    128,                        // noise mix
    0,                          // vol mod
    SD_USE_LFO | SD_USE_HPF,    // Flags
    // hho
    0,                          // freq
    0,                          // pitch mod
    0,                          // osc mix
    255,                        // noise mix
    160,                        // vol mod
    SD_USE_HPF,                 // Flags
    // cow
    SD_PACK_FREQ_WF(OSC_SQUARE, 20),    // freq
    255,                        // pitch mod
    180,                        // osc mix
    0,                          // noise mix
    10,                         // vol mod
    0,                          // Flags
    // crs
    0,                          // freq
    0,                          // pitch mod
    0,                          // osc mix
    255,                        // noise mix
    210,                        // vol mod
    SD_USE_HPF | SD_HPF_LOW_FREQ,       // Flags
    // rim
    SD_PACK_FREQ_WF(OSC_SQUARE, 1),     // freq
    0,                          // pitch mod
    128,                        // osc mix
    128,                        // noise mix
    0,                          // vol mod
    0,                          // Flags
    // ht
    SD_PACK_FREQ_WF(OSC_TRIANGLE, 25),  // freq
    150,                        // pitch mod
    192,                        // osc mix
    20,                         // noise mix
    140,                        // vol mod
    0,                          // Flags
};

#define ENV_MAX_LEN 0.9995
#define EXP_ENV(x) ENV_MAX_LEN + (1 - ENV_MAX_LEN) * x
#define uint8_to_f(x) (x / 255.0)

static inline void _set_syn_drum_params(const uint8_t *params)
{
    const uint8_t flags = params[SD_FLAGS_IDX] ^ variation;
    syn_drum_osc_type = SD_WAVEFORM(params[SD_FREQ_WF_IDX]);
    syn_drum_osc.frequency = (SD_FREQ(params[SD_FREQ_WF_IDX]) * 32) / (float) k_samplerate;
    syn_drum_osc.phase = noise < 0 ? -noise : noise;
    syn_drum_pitch_mod = EXP_ENV(uint8_to_f(params[SD_PITCH_MOD_IDX]));
    syn_drum_osc_mix = uint8_to_f(params[SD_OSC_MIX_IDX]);
    syn_drum_noise_mix = uint8_to_f(params[SD_NOISE_MIX_IDX]);
    syn_drum_vol_mod = EXP_ENV(uint8_to_f(params[SD_VOL_MOD_IDX]));
    syn_drum_vol = 1;
    use_hpf = flags & (SD_USE_HPF | SD_HPF_LOW_FREQ);
    if (flags & (SD_USE_HPF | SD_HPF_LOW_FREQ))
        hpf.factor = 0.7180302001078813; // 3 kHz
    else if (flags & SD_HPF_LOW_FREQ)
        hpf.factor = 0.8842517205783795; // 1 kHz
    else
        hpf.factor = 0.5600991537930968; // 6 kHz
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
    _set_syn_drum_params(&syn_drum_params[drum_idx * 6]);
    syn_drum_osc.frequency *= freq_modifier;
}

void set_syn_drum_variation(uint8_t new_variation)
{
    variation = new_variation;
}

#ifdef GENWAVES

uint8_t params[SD_FLAGS_IDX + 1];
int freq = 0, waveform = (int) OSC_SAW;

void read_params(FILE *f, const char *section)
{
    memset(params, 0, sizeof(params));
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
            params[SD_PITCH_MOD_IDX] = atoi(val);
        else if (!strcmp("osc_mix", cmd))
            params[SD_OSC_MIX_IDX] = atoi(val);
        else if (!strcmp("noise_mix", cmd))
            params[SD_NOISE_MIX_IDX] = atoi(val);
        else if (!strcmp("vol_mod", cmd))
            params[SD_VOL_MOD_IDX] = atoi(val);
        else if (!strcmp("flags", cmd) && !strcmp("SD_USE_HPF", val))
            params[SD_FLAGS_IDX] |= SD_USE_HPF;
        else if (!strcmp("flags", cmd) && !strcmp("SD_USE_LFO", val))
            params[SD_FLAGS_IDX] |= SD_USE_LFO;
        else if (!strcmp("flags", cmd) && !strcmp("SD_HPF_LOW_FREQ", val))
            params[SD_FLAGS_IDX] |= SD_HPF_LOW_FREQ;
        else if (!strcmp("variation", cmd))
        {
            variation = atoi(val);
        }
        else
            printf("Could not parse config line '%s'\n", buf);
    }
    params[SD_FREQ_WF_IDX] = SD_PACK_FREQ_WF(waveform, freq);
}

void params_to_code()
{
    if (params[SD_FREQ_WF_IDX] == 0)
        printf("0, // freq\n");
    else
        printf("SD_PACK_FREQ_WF(%s, %d)\n", waveform == (int) OSC_TRIANGLE
               ? "OSC_TRIANGLE" : (waveform == (int) OSC_SQUARE ? "OSC_SQUARE" : "OSC_SAW"), freq);
    printf("%u, // pitch mod\n", params[SD_PITCH_MOD_IDX]);
    printf("%u, // osc mix\n", params[SD_OSC_MIX_IDX]);
    printf("%u, // noise mix\n", params[SD_NOISE_MIX_IDX]);
    printf("%u, // vol mod\n", params[SD_VOL_MOD_IDX]);
    printf("0");
    if (params[SD_FLAGS_IDX] & SD_USE_HPF)
        printf(" | SD_USE_HPF");
    if (params[SD_FLAGS_IDX] & SD_USE_LFO)
        printf(" | SD_USE_LFO");
    if (params[SD_FLAGS_IDX] & SD_HPF_LOW_FREQ)
        printf(" | SD_HPF_LOW_FREQ");
    printf(" // flags\n");
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
    _set_syn_drum_params(params);
    for (int i = 0; i < k_samplerate * 4; i++)
    {
        float v = process_syn_drum();
        wav_set_normalized(&wav, i, &v);
    }
    write_wav_file(argv[2], &wav);      // "/mnt/shared/output.wav"
    free_wav_file(&wav);
    params_to_code();
    return 0;
}
#endif
