#pragma once

struct filter_state
{
    float factor;
    float ifactor;
    float state0, state1;
};

void init_filter(struct filter_state *st, float cutoff, float sample_rate);

float process_filter(struct filter_state *st, float input);

void init_hp_filter(struct filter_state *st, float cutoff, float sample_rate);

float process_hp_filter(struct filter_state *st, float input);
