#include "asymcompr.h"

static inline float compress(float input, float threshold, float ratio)
{
    return threshold + (input - threshold) * ratio;
}

inline float process_asymcompr(const AsymComprParams *p, float input)
{
    if (input > p->threshold0)
    {
        return compress(input, p->threshold0, p->ratio0);
    }
    if (input < p->threshold1)
    {
        return compress(input, p->threshold1, p->ratio1);
    }
    return input;
}
