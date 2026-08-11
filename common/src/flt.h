#pragma once

struct filter_state
{
    float factor;
#ifndef FLT_NO_LPF
    float ifactor;
#endif
    float state0
#ifndef FLT_NO_HPF
    ,state1
#endif
;
};

void init_filter(struct filter_state *st, float cutoff, float sample_rate);

float process_filter(struct filter_state *st, float input);

void init_hp_filter(struct filter_state *st, float cutoff, float sample_rate);

float process_hp_filter(struct filter_state *st, float input);
