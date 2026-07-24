#include "envelope_stage.h"

void init_EnvelopeStage(EnvelopeStage *es, int hasLength)
{
    es->length = hasLength ? 0 : -1;
    es->phase = 0;
}

void EnvelopeStage_setLength(EnvelopeStage *es, int samples)
{
    if (es->length >= 0 && samples >= 0)
    {
        es->length = samples;
    }
}

inline void EnvelopeStage_calcuateNext(EnvelopeStage *es)
{
    if (++es->phase >= es->length)
    {
        es->phase = es->length;
        es->ratio = 1;
    }
    else
    {
        es->ratio = es->phase / (float)es->length;
    }
}

inline int EnvelopeStage_hasNext(EnvelopeStage *es)
{
    return es->length == -1 || es->phase < es->length;
}

inline float EnvelopeStage_getRatio(EnvelopeStage *es)
{
    return es->ratio;
}

void EnvelopeStage_reset(EnvelopeStage *es)
{
    es->phase = 0;
    es->ratio = 0;
}