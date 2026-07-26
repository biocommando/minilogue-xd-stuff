#include "userosc.h"
#include "basic_oscillator.h"
#include "waveforms.h"
#include "waveform_random_orders.h"

static BasicOscillator osc, osc2, lfo;
static float window = 1, pos = 0, wt_param = 0;
static int ordering = 0;
static enum OscType lfo_type = OSC_TRIANGLE;
static float lfo_int = 0;
static uint32_t sticky = 0, sticky_counter = 0;

static inline void update_wt(BasicOscillator *o, float selection)
{
	int isel = selection * NUMBER_OF_WAVEFORMS;
	while (isel >= NUMBER_OF_WAVEFORMS) isel -= NUMBER_OF_WAVEFORMS;
	while (isel < 0) isel += NUMBER_OF_WAVEFORMS;
	isel = get_index_mapping(ordering, isel);
	BasicOscillator_setWavetable(o, (float*)get_waveform(isel), WAVEFORM_LENGTH);
}

void OSC_INIT(uint32_t platform, uint32_t api)
{
  (void)platform;
  (void)api;
  init_BasicOscillator(&osc, k_samplerate);
  init_BasicOscillator(&lfo, k_samplerate);
  update_wt(&osc, 0);
  BasicOscillator_setWaveTableParams(&osc, 0, 1);
}

static inline void update_inc(const user_osc_param_t *const params)
{
    const float inc = osc_w0f_for_note((params->pitch) >> 8, params->pitch & 0xFF);
    BasicOscillator_setFrequency(&osc, inc);
}

void OSC_CYCLE(const user_osc_param_t *const params,
               int32_t *yn,
               const uint32_t frames)
{
  update_inc(params);

  const float shape_lfo = q31_to_f32(params->shape_lfo);
  // No point in setting any osc2 values elsewhere as osc2 always
  // starts as a carbon copy of osc.
  memcpy(&osc2, &osc, sizeof(osc));
  const float lfo_val = BasicOscillator_getValue(&lfo, lfo_type) * lfo_int;
  if (sticky_counter >= sticky)
  {
	update_wt(&osc, wt_param + shape_lfo + lfo_val);
	sticky_counter = 0;
  }

  q31_t *__restrict y = (q31_t *)yn;
  const q31_t *y_e = y + frames;
  const float mix_inc = 1.0 / frames;
  float mix = 0;

  for (; y != y_e;)
  {
	BasicOscillator_calculateNext(&osc);
	BasicOscillator_calculateNext(&osc2);
	BasicOscillator_calculateNext(&lfo);
    const float o1 = BasicOscillator_getValue(&osc, OSC_WT);
    const float o2 = BasicOscillator_getValue(&osc2, OSC_WT);
    mix += mix_inc;
	
    const float out = o1 * mix + o2 * (1 - mix);
    *(y++) = f32_to_q31(out);
  }
  sticky_counter += frames;
}

void OSC_NOTEON(const user_osc_param_t *const params)
{
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
    pos = value / 100.0;
    BasicOscillator_setWaveTableParams(&osc, pos, window);
    break;
  case k_user_osc_param_id2:
    ordering = value % ORDER_MAP_SIZE;
    break;
  case k_user_osc_param_id3:
    lfo_int = value / 100.0;
    break;
  case k_user_osc_param_id4:
    BasicOscillator_setFrequency(&lfo, (value + 1) / 100.0 / k_samplerate);
    break;
  case k_user_osc_param_id5:
    if (value == 0)
		lfo_type = OSC_TRIANGLE;
    else if (value == 1)
		lfo_type = OSC_SINE;
    else if (value == 2)
		lfo_type = OSC_SAW;
    else if (value == 3)
		lfo_type = OSC_SQUARE;
    break;
  case k_user_osc_param_id6:
    sticky = k_samplerate / 100 * value;
    break;
  case k_user_osc_param_shape:
	{
		wt_param = param_val_to_f32(value) * 0.99;
	}
    break;
  case k_user_osc_param_shiftshape:
	{
		window =  0.05 + 0.95 * (1 - param_val_to_f32(value));
		BasicOscillator_setWaveTableParams(&osc, pos, window);
	}
    break;
  default:
    break;
  }
}
