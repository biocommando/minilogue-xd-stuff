#include "flt.h"

#ifndef FLT_NO_LPF
void init_filter(struct filter_state *st, float cutFreqHz, float sample_rate)
{
    st->factor = 1.0f / (1.0f / (2.0f * 3.14159265f * 1.0f / sample_rate * cutFreqHz) + 1.0f);
    st->ifactor = 1.0f - st->factor;
    st->state0 = 0;
}

inline float process_filter(struct filter_state *st, float input)
{
    st->state0 = st->factor * input + st->ifactor * st->state0;

    return st->state0;
}
#endif

#ifndef FLT_NO_HPF
void init_hp_filter(struct filter_state *st, float cutoff, float sample_rate)
{
	st->factor = 1.0f / (2.0f * 3.14159265f * 1.0f / sample_rate * cutoff + 1.0f);
    st->state0 = st->state1 = 0;
}

inline float process_hp_filter(struct filter_state *st, float input)
{
	st->state0 = st->factor * (input + st->state0 - st->state1);
	st->state1 = input;
	return st->state0;	
}
#endif
