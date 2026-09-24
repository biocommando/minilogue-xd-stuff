#include "userosc.h"
#include "stuff_util.h"

static inline uint32_t _synth_random()
{
    static uint32_t state = 123;
    state = (state >> 5) ^ state;
    state = (state << 7) ^ state;
    return state;
}

static uint32_t pw, phase_reset_thd, phase;

void OSC_CYCLE(const user_osc_param_t *const params, int32_t *yn, const uint32_t frames)
{
    const uint8_t note = (params->pitch >> 8) & 0x7f;
    const uint32_t inc = linintf(
        (params->pitch & 0xff) * k_note_mod_fscale,
        midi_to_hz_lut_f[note],
        midi_to_hz_lut_f[note + 1]) * 89478.4853125f;

    OSC_LOOP(y, yn, frames)
    {
        phase = phase + inc;

        if (_synth_random() < phase_reset_thd)
            phase = 0;
        *(y++) = phase > pw + (params->shape_lfo >> 1) ? 0x7fffffff : 0x80000000;
    }
}

void OSC_PARAM(uint16_t index, uint16_t value)
{
    if (index == k_user_osc_param_shape)
        phase_reset_thd = value << 15;
    else
        pw = value << 21;
}
