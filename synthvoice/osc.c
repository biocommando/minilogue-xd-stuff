#include "adsr_envelope.h"
#include "basic_oscillator.h"
#include "moog_filter.h"
#include "userosc.h"

static AdsrEnvelope env;
static MicrotrackerMoog filter;
static BasicOscillator osc, sub;

static enum OscType waveform = OSC_TRIANGLE;
static float sub_mix = 0;

void OSC_INIT(uint32_t platform, uint32_t api)
{
  (void)platform;
  (void)api;
  init_AdsrEnvelope(&env);
  init_MicrotrackerMoog(&filter, k_samplerate);
  init_BasicOscillator(&osc, k_samplerate);
}

static inline void update_inc(const user_osc_param_t *const params)
{
    const float inc = osc_w0f_for_note((params->pitch) >> 8, params->pitch & 0xFF);
    BasicOscillator_setFrequency(&osc, inc);
    BasicOscillator_setFrequency(&sub, inc * 0.5);
}

void OSC_CYCLE(const user_osc_param_t *const params,
               int32_t *yn,
               const uint32_t frames)
{

  update_inc(params);

  const float shape_lfo = q31_to_f32(params->shape_lfo);

  q31_t *__restrict y = (q31_t *)yn;
  const q31_t *y_e = y + frames;

  for (; y != y_e;)
  {
    AdsrEnvelope_calculateNext(&env);
    float mod = AdsrEnvelope_getEnvelope(&env);
    mod += shape_lfo;
    MicrotrackerMoog_setModulation(&filter, mod);
    BasicOscillator_calculateNext(&osc);
    BasicOscillator_calculateNext(&sub);
    float out = BasicOscillator_getValue(&osc, waveform);
    out += BasicOscillator_getValue(&sub, waveform) * sub_mix;
    out = MicrotrackerMoog_calculate(&filter, out);

    *(y++) = f32_to_q31(out);
  }
}

void OSC_NOTEON(const user_osc_param_t *const params)
{
  update_inc(params);
  AdsrEnvelope_trigger(&env);
}

void OSC_NOTEOFF(const user_osc_param_t *const params)
{
  (void)params;
  AdsrEnvelope_release(&env);
}


void OSC_PARAM(uint16_t index, uint16_t value)
{
  switch (index)
  {
  case k_user_osc_param_id1:
    AdsrEnvelope_setAttack(&env, 4 * k_samplerate * (value / 100.0));
    break;
  case k_user_osc_param_id2:
    AdsrEnvelope_setDecay(&env, 4 * k_samplerate * (value / 100.0));
    break;
  case k_user_osc_param_id3:
    AdsrEnvelope_setSustain(&env, value / 100.0);
    break;
  case k_user_osc_param_id4:
    AdsrEnvelope_setRelease(&env, 4 * k_samplerate * (value / 100.0));
    break;
  case k_user_osc_param_id5:
    if (value == 0) waveform = OSC_SAW;
    else if (value == 1) waveform = OSC_SQUARE;
    else if (value == 2) waveform = OSC_TRIANGLE;
    else if (value == 3) waveform = OSC_SINE;
    break;
  case k_user_osc_param_id6:
    sub_mix = value / 100.0;
    break;
  case k_user_osc_param_shape:
    MicrotrackerMoog_setCutoff(&filter, param_val_to_f32(value));
    break;
  case k_user_osc_param_shiftshape:
    MicrotrackerMoog_setResonance(&filter, param_val_to_f32(value));
    break;
  default:
    break;
  }
}
