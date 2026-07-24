#include "flt.h"

void init_filter(struct filter_state *st, float cutFreqHz, float sample_rate)
{
    st->factor = 1.0f / (1.0f / (2.0f * 3.14159265f * 1.0f / sample_rate * cutFreqHz) + 1.0f);
    st->a = st->b = 0;
}

float process_filter(struct filter_state *st, float input)
{
    st->a = st->factor * input + (1.0f - st->factor) * st->b;
    st->b = st->a;

    return st->a;
}