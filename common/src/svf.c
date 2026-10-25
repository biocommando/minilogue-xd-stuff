#include "svf.h"
#include "float_math.h"

void init_svf(struct svf_state *st, float cutoff, float sample_rate, float Q)
{
    st->bp = st->hp = st->lp = 0;
    st->f = 2 * fastsinfullf(M_PI * cutoff / sample_rate);
    st->q = 1.0f / Q;
}

void process_svf(struct svf_state *st, float input)
{
    st->lp += st->f * st->bp;
    st->hp = input - st->lp - st->q * st->bp;
    st->bp += st->f * st->hp;
}
