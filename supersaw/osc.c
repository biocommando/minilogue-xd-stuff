#include "userosc.h"
#include "synth_random.h"
#include "manifest_params.h"
#include "simple_oscillator.h"
#include "lookups.h"

#define N_OSC 13

static SimpleOscillator oscs[N_OSC];
float orig_freqs[N_OSC];
float osc_vol[N_OSC];

static float drift_p = 0;
static uint32_t drift_acc[N_OSC];

static float ph_rand = 0;

static uint8_t first_note_on = 0;
static uint32_t synth_random_init_value = 1;
uint8_t n_oscs = N_OSC;

#define RANDOM() ((synth_random()&0xFFFFF)/(float)0xFFFFF)

void OSC_INIT(uint32_t platform, uint32_t api)
{
    (void)platform;
    (void)api;
    memset(oscs, 0, sizeof(oscs));
    for (int i = 0; i < N_OSC; i++)
    {
        orig_freqs[i] = 1;
    }
}

__fast_inline void update_inc(const user_osc_param_t *const params, uint32_t frames)
{
    float inc = osc_w0f_for_note((params->pitch) >> 8, params->pitch & 0xFF);
    for (int i = 0; i < N_OSC; i++)
    {
        const float drift = calculate_drift(&drift_acc[i], frames);
        // Max drift +-1 semitone.
        // Magic numbers:
        // 2**(1/12) - 1/(2**(1/12)) = 0.11558878 (whole range)
        // 1/(2**(1/12)) = 0.94387432 (range min)
        oscs[i].frequency = inc * orig_freqs[i] * (0.94387432 + drift * 0.11558878 * drift_p);
    }
}

__fast_inline float calculate_supersaw()
{
    float out = 0;
    for (int i = 0; i < n_oscs; i++)
    {
        SimpleOscillator_calculateNext(&oscs[i]);
        out += SimpleOscillator_getValue(&oscs[i], OSC_SAW) * osc_vol[i];
    }
    return out;
}

void OSC_CYCLE(const user_osc_param_t *const params,
               int32_t *yn,
               const uint32_t frames)
{
  synth_random_init_value += frames;
  update_inc(params, frames);

  q31_t *__restrict y = (q31_t *)yn;
  const q31_t *y_e = y + frames;

  for (; y != y_e;)
  {
    *(y++) = f32_to_q31(calculate_supersaw());
  }
}

void OSC_NOTEON(const user_osc_param_t *const params)
{
    (void)params;
    if (!first_note_on)
    {
        first_note_on = 1;
        synth_random_reset(synth_random_init_value);
    }
    for (int i = 0; i < N_OSC; i++) {
        oscs[i].phase = RANDOM() * ph_rand;
        drift_acc[i] = synth_random();
    }
}

void OSC_NOTEOFF(const user_osc_param_t *const params)
{
    (void)params;
}

static float initial_vol = 0.5, amp_dist;
static void calc_volume()
{
    osc_vol[0] = initial_vol;
    float v = amp_dist;
    for (int i = 1; i < N_OSC; i++)
    {
        osc_vol[i] = initial_vol * v;
        if (i % 2 == 0)
            v *= v;
    }
}

void OSC_PARAM(uint16_t index, uint16_t value)
{
  switch (index)
  {
  case USER_PARAM__Amp_dist__idx:
    {
        amp_dist = 1 - value / 100.0;
        calc_volume();
    }
    break;
  case USER_PARAM__Attenuation__idx:
    {
        initial_vol = 0.5 - 0.45 * value / 100.0;
        calc_volume();
    }
    break;
  case USER_PARAM__Ph_rand_mod__idx:
    {
        ph_rand = 1 - value / 100.0;
    }
    break;
  case USER_PARAM__Num_oscs__idx:
    {
        if (value == 0)
            n_oscs = 7;
        else if (value == 1)
            n_oscs = 9;
        else if (value == 2)
            n_oscs = 11;
        else if (value == 3)
            n_oscs = 13;
    }
    break;
  case k_user_osc_param_shape:
    {
        for (int i = 1; i < N_OSC; i++)
        {
            uint16_t spread = value * ((i+1)/2/(float)((N_OSC-1)/2));
            float f = get_semitone_ratio(spread);
            if (i & 1)
                f = 1/f;
            orig_freqs[i] = f;
        }
    }
    break;
  case k_user_osc_param_shiftshape:
    drift_p  = param_val_to_f32(value);
    break;
  default:
    break;
  }
}
