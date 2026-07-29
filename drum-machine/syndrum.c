#include "syndrum.h"
#include "userosc.h"
#include "simple_oscillator.h"

static SimpleOscillator syn_drum_osc = { .phase = 0, .frequency = 0.0001 };
static float syn_drum_pitch_mod;
static float syn_drum_vol, syn_drum_vol_mod;
static float syn_drum_osc_mix, syn_drum_noise_mix;
static enum OscType syn_drum_osc_type;

// Fast pseudo random implementation
static uint32_t rand_state = 123;
static inline uint32_t next_random()
{
  rand_state ^= rand_state << 13;
  rand_state ^= rand_state >> 17;
  rand_state ^= rand_state << 5;
  return rand_state;
}

float process_syn_drum()
{
    SimpleOscillator_calculateNext(&syn_drum_osc);
    float v = SimpleOscillator_getValue(&syn_drum_osc, syn_drum_osc_type) * syn_drum_osc_mix;
    v += (0x7fffffff - (int)next_random())/(float)(0x7fffffff) * syn_drum_noise_mix;
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

#define SD_FREQ(x) ((x) & 0x1f)
#define SD_WAVEFORM(x) ((enum OscType)(((x)>>6) & 3))

#define SD_PACK_FREQ_WF(waveform, frequency) ((((uint8_t)(waveform)) << 6) | (frequency))

static const uint8_t syn_drum_params[] = {
    // kick 1
        SD_PACK_FREQ_WF(OSC_TRIANGLE, 8), // freq
        140, // pitch mod
        255, // osc mix
        0,  // noise mix
        150, // vol mod
    // kick 2
        SD_PACK_FREQ_WF(OSC_TRIANGLE, 10), // freq
        180, // pitch mod
        255, // osc mix
        0,  // noise mix
        150, // vol mod
    // snare 1
        0, // freq
        0, // pitch mod
        0, // osc mix
        255,  // noise mix
        70, // vol mod
    // snare 2
        SD_PACK_FREQ_WF(OSC_SQUARE, 31), // freq
        200, // pitch mod
        90, // osc mix
        255,  // noise mix
        100, // vol mod
    // hcp
        SD_PACK_FREQ_WF(OSC_SQUARE, 20), // freq
        80, // pitch mod
        50, // osc mix
        255,  // noise mix
        120, // vol mod
    // hhc
        0, // freq
        0, // pitch mod
        0, // osc mix
        255,  // noise mix
        0, // vol mod
    // tam
        SD_PACK_FREQ_WF(OSC_SAW, 31), // freq
        252, // pitch mod
        128, // osc mix
        64,  // noise mix
        0, // vol mod
    // hho
        0, // freq
        0, // pitch mod
        0, // osc mix
        255,  // noise mix
        70, // vol mod
    // cow
        SD_PACK_FREQ_WF(OSC_SQUARE, 20), // freq
        255, // pitch mod
        180, // osc mix
        0,  // noise mix
        10, // vol mod
    // crs
        0, // freq
        0, // pitch mod
        0, // osc mix
        255,  // noise mix
        210, // vol mod
    // rim
        SD_PACK_FREQ_WF(OSC_SQUARE, 1), // freq
        0, // pitch mod
        128, // osc mix
        128,  // noise mix
        0, // vol mod
    // ht
        SD_PACK_FREQ_WF(OSC_TRIANGLE, 25), // freq
        150, // pitch mod
        192, // osc mix
        20,  // noise mix
        140 // vol mod
};

#define ENV_MAX_LEN 0.9995
#define EXP_ENV(x) ENV_MAX_LEN + (1 - ENV_MAX_LEN) * x
#define uint8_to_f(x) (x / 255.0)

static inline void _set_syn_drum_params(const uint8_t *params)
{
    syn_drum_osc_type = SD_WAVEFORM(params[SD_FREQ_WF_IDX]);
    syn_drum_osc.frequency = (SD_FREQ(params[SD_FREQ_WF_IDX]) * 32) / (float)k_samplerate;
    // Osc free running... to decrease the phasing effect
    //syn_drum_osc.phase = next_random() / (float)0xFFFFFFFF;
    syn_drum_pitch_mod = EXP_ENV(uint8_to_f(params[SD_PITCH_MOD_IDX]));
    syn_drum_osc_mix = uint8_to_f(params[SD_OSC_MIX_IDX]);
    syn_drum_noise_mix = uint8_to_f(params[SD_NOISE_MIX_IDX]);
    syn_drum_vol_mod = EXP_ENV(uint8_to_f(params[SD_VOL_MOD_IDX]));
    syn_drum_vol = 1;
}

void set_syn_drum_params(int drum_idx, float freq_modifier)
{
	_set_syn_drum_params(&syn_drum_params[drum_idx * 5]);
	syn_drum_osc.frequency *= freq_modifier;
}
