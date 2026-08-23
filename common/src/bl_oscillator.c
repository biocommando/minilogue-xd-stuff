#include "bl_oscillator.h"
#include "userosc.h"

inline static float tri1(float phase)
{
    if (phase < 0.5)
        return 4 * phase - 1;
    else
        return -4 * phase + 3;
}

inline static float saw1(float phase, float idx)
{
    return osc_bl_sawf(phase, idx);
}

inline static float sqr1(float phase, float idx)
{
    return osc_bl_sqrf(phase, idx);
}

void BlOscillator_calculateNext(BlOscillator *bo)
{
    bo->phase = bo->phase + bo->frequency;
    if (bo->phase >= 1.0)
    {
        bo->phase = bo->phase - 1.0f;
    }
}

inline static float get_value(float p, enum OscType oscType, uint8_t saw_idx, uint8_t sqr_idx)
{
    switch (oscType)
    {
        case OSC_TRIANGLE:
            return tri1(p);
        case OSC_SAW:
            return saw1(p, saw_idx);
        case OSC_SQUARE:
        default:
            return sqr1(p, sqr_idx);
    }
}

float BlOscillator_getValue(BlOscillator *bo, enum OscType oscType)
{
    return get_value(bo->phase, oscType, bo->note_idx_saw, bo->note_idx_sqr);
}

void BlOscillator_setFrequency(BlOscillator * bo, float inc, uint8_t note)
{
    bo->frequency = inc;
    bo->note_idx_saw = osc_bl_saw_idx(note);
    bo->note_idx_sqr = osc_bl_sqr_idx(note);
}
