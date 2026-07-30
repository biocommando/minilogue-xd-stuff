#include "syndrum.h"
#include "userosc.h"
#include "simple_oscillator.h"
#include "flt.h"

static SimpleOscillator syn_drum_osc = { .phase = 0, .frequency = 0.0001 };
static SimpleOscillator syn_drum_lfo = { .phase = 0, .frequency = 25.0 / k_samplerate };
static float syn_drum_pitch_mod;
static float syn_drum_vol, syn_drum_vol_mod;
static float syn_drum_osc_mix, syn_drum_noise_mix;
static enum OscType syn_drum_osc_type;

static uint8_t use_hpf, use_lfo;
// Fixed high pass filter at 6 kHz
static struct filter_state hpf = {.factor = 0.5600991537930968, .state0 = 0, .state1 = 0};

// Fast pseudo random implementation
static uint32_t rand_state = 123;
static uint32_t next_random()
{
  rand_state ^= rand_state << 13;
  rand_state ^= rand_state >> 17;
  rand_state ^= rand_state << 5;
  return rand_state;
}
static float noise; // last noise value (-1..1) saved here to prevent the need for rng calls elsewhere
float process_syn_drum()
{
    SimpleOscillator_calculateNext(&syn_drum_osc);
    float v = SimpleOscillator_getValue(&syn_drum_osc, syn_drum_osc_type);
    v *= syn_drum_osc_mix;
    noise = (0x7fffffff - (int)next_random())/(float)(0x7fffffff);
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
        0, // Flags
    // kick 2
        SD_PACK_FREQ_WF(OSC_TRIANGLE, 10), // freq
        180, // pitch mod
        255, // osc mix
        0,  // noise mix
        150, // vol mod
        0, // Flags
    // snare 1
        0, // freq
        0, // pitch mod
        0, // osc mix
        255,  // noise mix
        70, // vol mod
        0, // Flags
    // snare 2
        SD_PACK_FREQ_WF(OSC_SQUARE, 5), // freq
        200, // pitch mod
        32, // osc mix
        255,  // noise mix
        100, // vol mod
        0, // Flags
    // hcp
        SD_PACK_FREQ_WF(OSC_SQUARE, 20), // freq
        80, // pitch mod
        50, // osc mix
        255,  // noise mix
        120, // vol mod
        SD_USE_LFO | SD_USE_HPF | SD_HPF_LOW_FREQ, // Flags
    // hhc
        0, // freq
        0, // pitch mod
        0, // osc mix
        255,  // noise mix
        0, // vol mod
        SD_USE_HPF, // Flags
    // tam
        SD_PACK_FREQ_WF(OSC_SAW, 31), // freq
        252, // pitch mod
        255, // osc mix
        128,  // noise mix
        0, // vol mod
        SD_USE_LFO | SD_USE_HPF, // Flags
    // hho
        0, // freq
        0, // pitch mod
        0, // osc mix
        255,  // noise mix
        160, // vol mod
        SD_USE_HPF, // Flags
    // cow
        SD_PACK_FREQ_WF(OSC_SQUARE, 20), // freq
        255, // pitch mod
        180, // osc mix
        0,  // noise mix
        10, // vol mod
        0, // Flags
    // crs
        0, // freq
        0, // pitch mod
        0, // osc mix
        255,  // noise mix
        210, // vol mod
        SD_USE_HPF | SD_HPF_LOW_FREQ, // Flags
    // rim
        SD_PACK_FREQ_WF(OSC_SQUARE, 1), // freq
        0, // pitch mod
        128, // osc mix
        128,  // noise mix
        0, // vol mod
        0, // Flags
    // ht
        SD_PACK_FREQ_WF(OSC_TRIANGLE, 25), // freq
        150, // pitch mod
        192, // osc mix
        20,  // noise mix
        140, // vol mod
        0, // Flags
};

#define ENV_MAX_LEN 0.9995
#define EXP_ENV(x) ENV_MAX_LEN + (1 - ENV_MAX_LEN) * x
#define uint8_to_f(x) (x / 255.0)

static inline void _set_syn_drum_params(const uint8_t *params)
{
    syn_drum_osc_type = SD_WAVEFORM(params[SD_FREQ_WF_IDX]);
    syn_drum_osc.frequency = (SD_FREQ(params[SD_FREQ_WF_IDX]) * 32) / (float)k_samplerate;
    syn_drum_osc.phase = noise < 0 ? -noise : noise;
    syn_drum_pitch_mod = EXP_ENV(uint8_to_f(params[SD_PITCH_MOD_IDX]));
    syn_drum_osc_mix = uint8_to_f(params[SD_OSC_MIX_IDX]);
    syn_drum_noise_mix = uint8_to_f(params[SD_NOISE_MIX_IDX]);
    syn_drum_vol_mod = EXP_ENV(uint8_to_f(params[SD_VOL_MOD_IDX]));
    syn_drum_vol = 1;
    use_hpf = params[SD_FLAGS_IDX] & SD_USE_HPF;
    if (params[SD_FLAGS_IDX] & SD_HPF_LOW_FREQ)
		hpf.factor = 0.8842517205783795;
	else
		hpf.factor = 0.5600991537930968;
	use_lfo = params[SD_FLAGS_IDX] & SD_USE_LFO;
	syn_drum_lfo.phase = 0;
}

void set_syn_drum_params(int drum_idx, float freq_modifier)
{
	_set_syn_drum_params(&syn_drum_params[drum_idx * 6]);
	syn_drum_osc.frequency *= freq_modifier;
}
