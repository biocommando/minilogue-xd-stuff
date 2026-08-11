#include "simple_oscillator.h"

inline static float tri1(float phase)
{
    if (phase < 0.5)
        return 4 * phase - 1;
    else
        return -4 * phase + 3;
}

inline static float saw1(float phase)
{
    return 2 * phase - 1;
}

inline static float sqr1(float phase)
{
    return phase < 0.5 ? 1.0f : -1.0f;
}

void SimpleOscillator_calculateNext(SimpleOscillator *so)
{
    so->phase = so->phase + so->frequency;
    if (so->phase >= 1.0)
    {
        so->phase = so->phase - 1.0f;
    }
}

inline static float get_value(float p, enum OscType oscType)
{
    switch (oscType)
    {
        case OSC_TRIANGLE:
            return tri1(p);
        case OSC_SAW:
            return saw1(p);
        case OSC_SQUARE:
        default:
            return sqr1(p);
    }
}

float SimpleOscillator_getValue(SimpleOscillator *so, enum OscType oscType)
{
    return get_value(so->phase, oscType);
}
