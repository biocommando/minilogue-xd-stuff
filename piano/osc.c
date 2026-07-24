#include "flt.h"
#include "dataoscsrc.h"
#include "userosc.h"

#define MIDDLE_C_FREQ_HZ 261.6256

#define RELEASE_STATE_INIT 0
#define RELEASE_STATE_RELEASED 1
#define RELEASE_STATE_ENDED 2

#define TAIL_LOOP_MODE_TO_END 0
#define TAIL_LOOP_MODE_INFINITE 1

#define INFINITE_BOUNCE 33

struct data_osc
{
  // Sample data array
  const int8_t *data;
  // Sample data number of frames
  uint32_t data_len;
  // Sample data sampling rate
  uint32_t data_sr;
  // Sample player index
  float phase;
  // Index increment
  float inc;
  // Release state (INIT = not released, RELEASED = released but not played to end, ENDED = voice should be silent after this)
  uint8_t release_state;
  // Point where loopback is triggered; effectively the length of the sample
  float loopback_point;
  // Length of the fraction played when looping the tail
  float loopback_offs;
  // What to do after note release? see TAIL_LOOP_MODE_TO_END, TAIL_LOOP_MODE_INFINITE
  uint8_t tail_loop_mode;
  // Configured number of loopback bounces; meaning that instead of looping the tail,
  // start playback at earlier index
  uint8_t n_loopback_bounce;
  // Number of bounces left. INFINITE_BOUNCE = never stop bouncing
  uint8_t loopback_bounce_counter;
  // Probability of jumping to playback index 0 instead of loopback_bounce_offset
  uint8_t loopback_bounce_full_reset_prob;
  // Where the playback index will jump to when bouncing when sample end reached.
  // Range 0..1. The position is loopback_point * loopback_bounce_offset
  float loopback_bounce_offset;
  // Bandwidth limiting filter
  struct filter_state bwlim;
  // Bandwidth limiting filter cutoff as a ratio in respect to note fundamental frequency.
  // E.g. bwlim_ratio = 10, Playing note at 440 Hz -> bwlim cutoff at 4400 Hz.
  float bwlim_ratio;
};

static struct data_osc osc;

// Fast pseudo random implementation
static uint32_t rand_state = 123;
static inline uint32_t next_random()
{
  rand_state ^= rand_state << 13;
  rand_state ^= rand_state >> 17;
  rand_state ^= rand_state << 5;
  return rand_state;
}

static inline uint32_t calc_loopback_point(float v)
{
  while (v > 1)
    v -= 1;
  while (v < 0)
    v += 1;
  const uint32_t offs = (uint32_t)osc.loopback_offs + 1;
  return offs + (osc.data_len - offs) * v;
}

static inline float calc_freq(const user_osc_param_t *const params)
{
  return osc_w0f_for_note((params->pitch) >> 8, params->pitch & 0xFF) / k_samplerate_recipf;
}

static inline float calc_inc(float freq)
{
  float freqratio = 1 / MIDDLE_C_FREQ_HZ * osc.data_sr / k_samplerate;
  return freq * freqratio;
}

static inline float data_osc_get()
{
  uint32_t i = osc.phase;
  if (i >= osc.data_len)
    return 0;
  return osc.data[i] / 127.0f;
}

void OSC_INIT(uint32_t platform, uint32_t api)
{
  (void)platform;
  (void)api;

  osc.data = get_waveform(&osc.data_len, &osc.data_sr);
  osc.loopback_offs = osc.data_sr / MIDDLE_C_FREQ_HZ;
  osc.bwlim_ratio = 20;
  osc.loopback_point = osc.data_len;
  osc.loopback_bounce_offset = 0;
  osc.loopback_bounce_full_reset_prob = 0;
}

void OSC_CYCLE(const user_osc_param_t *const params,
               int32_t *yn,
               const uint32_t frames)
{
  osc.inc = calc_inc(calc_freq(params));
  float shape_lfo = q31_to_f32(params->shape_lfo);

  uint32_t loopback_point = calc_loopback_point(osc.loopback_point + shape_lfo);

  q31_t *__restrict y = (q31_t *)yn;
  const q31_t *y_e = y + frames;

  for (; y != y_e;)
  {
    if (osc.release_state == RELEASE_STATE_ENDED)
    {
      *(y++) = 0;
      continue;
    }

    float out = data_osc_get();
    osc.phase += osc.inc;
    if (osc.phase >= loopback_point)
    {
      if (osc.loopback_bounce_counter > 0)
      {
        if (osc.loopback_bounce_full_reset_prob && (next_random() & 63) < osc.loopback_bounce_full_reset_prob)
          osc.phase = 0;
        else
          osc.phase = osc.loopback_bounce_offset * loopback_point;
        if (osc.loopback_bounce_counter < INFINITE_BOUNCE)
          osc.loopback_bounce_counter--;
      }
      else
      {
        osc.phase -= osc.loopback_offs;
      }
      if (osc.release_state == RELEASE_STATE_RELEASED && osc.tail_loop_mode == TAIL_LOOP_MODE_TO_END)
        osc.release_state = RELEASE_STATE_ENDED;
    }

    out = process_filter(&osc.bwlim, out);
    *(y++) = f32_to_q31(out);
  }
}

void OSC_NOTEON(const user_osc_param_t *const params)
{
  const float freq = calc_freq(params);

  float ffreq = freq * osc.bwlim_ratio;
  if (ffreq > k_samplerate * 0.4)
    ffreq = k_samplerate * 0.4;
  init_filter(&osc.bwlim, ffreq, k_samplerate);

  osc.inc = calc_inc(freq);
  osc.phase = 0;
  osc.release_state = RELEASE_STATE_INIT;
  osc.loopback_bounce_counter = osc.loopback_bounce_offset < 0.001 ? 0 : osc.n_loopback_bounce;
}

void OSC_NOTEOFF(const user_osc_param_t *const params)
{
  (void)params;
  osc.release_state = RELEASE_STATE_RELEASED;
}

void OSC_PARAM(uint16_t index, uint16_t value)
{
  switch (index)
  {
  case k_user_osc_param_id1:
    osc.tail_loop_mode = value == 0 ? TAIL_LOOP_MODE_TO_END : TAIL_LOOP_MODE_INFINITE;
    break;
  case k_user_osc_param_id2:
    osc.n_loopback_bounce = (uint8_t)value + 1;
    break;
  case k_user_osc_param_id3:
    osc.loopback_bounce_full_reset_prob = value;
    break;
  case k_user_osc_param_id4:
    if (value == 0)
      value = 20;
    osc.bwlim_ratio = value;
    break;
  case k_user_osc_param_shape:
    osc.loopback_point = param_val_to_f32(value);
    break;
  case k_user_osc_param_shiftshape:
    osc.loopback_bounce_offset = param_val_to_f32(value);
    break;
  default:
    break;
  }
}
