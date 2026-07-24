#include "synth_random.h"
#include "basic_oscillator.h"
#include "userosc.h"

extern const float *get_wt();
extern int get_wt_size();

#define N_OSC 5
static BasicOscillator osc[N_OSC];

static enum OscType waveform = OSC_TRIANGLE;
static float osc_mix[N_OSC] = {0,0,0,0,0};
static float ph_rand = 0.5;
static float noise_mix = 0;
static float wt_pos = 0;

void OSC_INIT(uint32_t platform, uint32_t api)
{
  (void)platform;
  (void)api;
  for (int i = 0; i < N_OSC; i++)
  {
    init_BasicOscillator(&osc[i], k_samplerate);
    BasicOscillator_setWavetable(&osc[i], (float*)get_wt(), get_wt_size());
  }
}

static inline void update_inc(const user_osc_param_t *const params)
{
    const float inc = osc_w0f_for_note((params->pitch) >> 8, params->pitch & 0xFF);
    const float inclist[N_OSC] = {
			1, 0.25, 0.5, 2, 4
	};
	for (int i = 0; i < N_OSC; i++)
	    BasicOscillator_setFrequency(&osc[i], inc * inclist[i]);
}

void OSC_CYCLE(const user_osc_param_t *const params,
               int32_t *yn,
               const uint32_t frames)
{

  update_inc(params);

  const float shape_lfo = q31_to_f32(params->shape_lfo);
  float mod_wt_pos = wt_pos + shape_lfo;
  while (mod_wt_pos > 1) mod_wt_pos -= 1;
  while (mod_wt_pos < 0) mod_wt_pos += 1;
  for (int i=0; i < N_OSC; i++)
    BasicOscillator_setWaveTableParams(&osc[i], mod_wt_pos, 0.25);

  q31_t *__restrict y = (q31_t *)yn;
  const q31_t *y_e = y + frames;

  for (; y != y_e;)
  {
    float out = 0;
    for (int i = 0; i < N_OSC; i++)
    {
      BasicOscillator_calculateNext(&osc[i]);
      out += BasicOscillator_getValue(&osc[i], waveform) * osc_mix[i];
    }
    out += (float)(synth_random() % 100000) * 0.00001f * noise_mix;
    out *= 1 - shape_lfo;

    *(y++) = f32_to_q31(out);
  }
}

void OSC_NOTEON(const user_osc_param_t *const params)
{
  for (int i = 0; i < N_OSC; i++)
    BasicOscillator_randomizePhase(&osc[i], ph_rand);
  update_inc(params);
}

void OSC_NOTEOFF(const user_osc_param_t *const params)
{
  (void)params;
}


void OSC_PARAM(uint16_t index, uint16_t value)
{
  switch (index)
  {
  case k_user_osc_param_id1:
    if (value == 0) waveform = OSC_SAW;
    else if (value == 1) waveform = OSC_SQUARE;
    else if (value == 2) waveform = OSC_TRIANGLE;
    else if (value == 3) waveform = OSC_SINE;
    else if (value == 4) waveform = OSC_WT;
    break;
  case k_user_osc_param_id2:
    osc_mix[0] = 1 - value / 100.0;
    break;
  case k_user_osc_param_id3:
    osc_mix[1] = value / 100.0;
    break;
  case k_user_osc_param_id4:
    osc_mix[2] = value / 100.0;
    break;
  case k_user_osc_param_id5:
    osc_mix[3] = value / 100.0;
    break;
  case k_user_osc_param_id6:
    osc_mix[4] = value / 100.0;
    break;
  case k_user_osc_param_shape:
      wt_pos = param_val_to_f32(value);
    break;
  case k_user_osc_param_shiftshape:
    noise_mix = param_val_to_f32(value);
    break;
  default:
    break;
  }
}
