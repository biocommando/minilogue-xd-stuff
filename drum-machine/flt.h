#pragma once

struct filter_state
{
    float factor;
    float state;
};

void init_filter(struct filter_state *st, float cutoff, float sample_rate);

float process_filter(struct filter_state *st, float input);
