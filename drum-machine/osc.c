#include "userosc.h"
#include "manifest_params.h"
#include "combined_waveforms.h"
#include "flt.h"
#include "syndrum.h"

#define NOTE_C 0
#define NOTE_C_SHARP 1
#define NOTE_D 2
#define NOTE_D_SHARP 3
#define NOTE_E 4
#define NOTE_F 5
#define NOTE_F_SHARP 6
#define NOTE_G 7
#define NOTE_G_SHARP 8
#define NOTE_A 9
#define NOTE_A_SHARP 10
#define NOTE_B 11

// Sample playback

struct compressed_osc {
    const uint16_t *data;
    uint16_t data_len;
    float phase;
    float inc;
};

static struct compressed_osc osc;

// Overdrive
static float gain = 1;
static struct filter_state pre_dist_flt;

static const float invert_threshold = 17;
static const float clip_threshold = 0.95;

// Gate mode
static uint8_t cut_at_noteoff = 0, note_off_received = 0;

// Mixing
static float sample_mix_vol[NUM_WAVEFORMS];
static float mix_vol;

// Synthesized drums
static float syn_drum_mix = 0;

///////////

static inline float compressed_osc_get(const struct compressed_osc *osc)
{
    int i = osc->phase;
    int ai = i / 5;
    int wi = i % 5;
    if (ai >= osc->data_len || ai < 0)
        return 0;
    uint16_t word = osc->data[ai];
    float s = ((int)((word >> (wi * 3)) & 0x7)) / 7.0f;
    return (word & 0x8000) ? -s : s;
}

void OSC_INIT(uint32_t platform, uint32_t api)
{
  (void)platform;
  (void)api;
  set_syn_drum_params(0, 1);
}

void OSC_CYCLE(const user_osc_param_t *const params,
               int32_t *yn,
               const uint32_t frames)
{
  const float shape_lfo = q31_to_f32(params->shape_lfo);// * 10;
  float mod_syn_drum_mix = syn_drum_mix + shape_lfo;
  if (mod_syn_drum_mix > 1) mod_syn_drum_mix = 1;
  else if (mod_syn_drum_mix < 0) mod_syn_drum_mix = 0;

  const float g1 = mod_syn_drum_mix < 0.5 ? 1 : 2 - 2 * mod_syn_drum_mix;
  const float g2 = mod_syn_drum_mix < 0.5 ? 2 * mod_syn_drum_mix : 1;

  q31_t *__restrict y = (q31_t *)yn;
  const q31_t *y_e = y + frames;

  for (; y != y_e;)
  {
    osc.phase += osc.inc;
    float out = compressed_osc_get(&osc) * g1 + process_syn_drum() * g2;
    out = process_filter(&pre_dist_flt, out * gain);
    if (out > invert_threshold || out < -invert_threshold) out *= -1;
    if (out > clip_threshold) out = clip_threshold;
    else if (out < -clip_threshold) out = -clip_threshold;
    
    out *= mix_vol;
    if (note_off_received)
        mix_vol *= 0.95;

    *(y++) = f32_to_q31(out);
  }
}

void OSC_NOTEON(const user_osc_param_t *const params)
{
  float base_freq = 44100.0 / k_samplerate; 
  const int pitch = (params->pitch) >> 8;
  const int note = pitch % 12;
  int octave = pitch / 12;
  float freq_octave = 0.5;
  while (octave--)
    freq_octave *= 1.148698354997035;

  int load_wave = 0;
  switch(note)
  {
    case NOTE_C:
      load_wave = WAVEFORM_ID_bd;
      break;
    case NOTE_C_SHARP:
      load_wave = WAVEFORM_ID_bd1;
      break;
    case NOTE_D:
      load_wave = WAVEFORM_ID_sd;
      break;
    case NOTE_D_SHARP:
      load_wave = WAVEFORM_ID_sd1;
      break;
    case NOTE_E:
      load_wave = WAVEFORM_ID_hcp;
      break;
    case NOTE_F:
      load_wave = WAVEFORM_ID_hhc;
      break;
    case NOTE_F_SHARP:
      load_wave = WAVEFORM_ID_tam;
      break;
    case NOTE_G:
      load_wave = WAVEFORM_ID_hho;
      break;
    case NOTE_G_SHARP:
      load_wave = WAVEFORM_ID_cow;
      break;
    case NOTE_A:
      load_wave = WAVEFORM_ID_crs;
      break;
    case NOTE_A_SHARP:
      load_wave = WAVEFORM_ID_rim;
      break;
    case NOTE_B:
      load_wave = WAVEFORM_ID_ht;
      break;
  }
  if (load_wave == WAVEFORM_ID_crs) // Crash sample with lower sample rate
      base_freq = 32000.0 / k_samplerate;
  osc.data = get_waveform(load_wave, &osc.data_len);
  set_syn_drum_params(note, freq_octave);

  osc.inc = base_freq * freq_octave;
  osc.phase = 0;
  mix_vol = sample_mix_vol[load_wave];
  note_off_received = 0;
}

void OSC_NOTEOFF(const user_osc_param_t *const params)
{
  (void)params;
  note_off_received = cut_at_noteoff;
}

static void set_mix_vol(uint8_t idx, uint16_t value)
{
  sample_mix_vol[idx] = 1.0 - value / 100.0;
}

void OSC_PARAM(uint16_t index, uint16_t value)
{
  switch (index)
  {
  case USER_PARAM__Gate_mode__idx:
    cut_at_noteoff = value ? 1 : 0;
    break;
  case USER_PARAM__Kick_cut__idx:
    set_mix_vol(WAVEFORM_ID_bd, value);
    set_mix_vol(WAVEFORM_ID_bd1, value);
    break;
  case USER_PARAM__Snare_cut__idx:
    set_mix_vol(WAVEFORM_ID_sd, value);
    set_mix_vol(WAVEFORM_ID_sd1, value);
    set_mix_vol(WAVEFORM_ID_hcp, value);
    break;
  case USER_PARAM__Hats_cut__idx:
    set_mix_vol(WAVEFORM_ID_hhc, value);
    set_mix_vol(WAVEFORM_ID_hho, value);
    break;
  case USER_PARAM__Crash_cut__idx:
    set_mix_vol(WAVEFORM_ID_crs, value);
    break;
  case USER_PARAM__Perc_cut__idx:
    set_mix_vol(WAVEFORM_ID_tam, value);
    set_mix_vol(WAVEFORM_ID_cow, value);
    set_mix_vol(WAVEFORM_ID_rim, value);
    set_mix_vol(WAVEFORM_ID_ht, value);
    break;
  case k_user_osc_param_shiftshape:
    {
      const float fval = param_val_to_f32(value);
      gain = 1.0 + fval * 20;
      init_filter(&pre_dist_flt, (1 - fval * 0.8) * 0.5 * k_samplerate, k_samplerate);
    }
    break;
  case k_user_osc_param_shape:
    syn_drum_mix = param_val_to_f32(value);
    break;
  default:
    break;
  }
}
