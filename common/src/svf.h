#pragma once

struct svf_state
{
    float f, q;
    float lp, hp, bp;
};

void init_svf(struct svf_state *st, float cutoff, float sample_rate, float Q);

void process_svf(struct svf_state *st, float input);
