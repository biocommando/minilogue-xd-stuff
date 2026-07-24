#pragma once

// Ported from C++ code

/*
 * One stage in an envelope. Basically just counts time.
 */
typedef struct
{
    // Stage length in samples. -1 = no length (e.g. a sustain stage)
    int length;
    // Stage sample index
    int phase;
    // Current stage complete ratio. 0 = stage at start, 1 = stage at end.
    // For infinite stages (length = -1) this is always 1.
    float ratio;
} EnvelopeStage;

/*
 * Sets the stage length in samples.
 */
void EnvelopeStage_setLength(EnvelopeStage *es, int samples);
/*
 * Proceeds the stage state by one sample.
 */
void EnvelopeStage_calcuateNext(EnvelopeStage *es);
/*
 * Returns 1 if the stage still continues according to the phase counter, 0 otherwise.
 */
int EnvelopeStage_hasNext(EnvelopeStage *es);
/*
 * Getter for ratio.
 */
float EnvelopeStage_getRatio(EnvelopeStage *es);
/*
 * Resets the envelope stage to the beginning of the stage.
 */
void EnvelopeStage_reset(EnvelopeStage *es);
/*
 * Initializes an envelope stage. The parameter hasLength determines
 * if the stage is infinitely long (e.g. a sustain stage) or if it
 * has an end.
 */
void init_EnvelopeStage(EnvelopeStage *es, int hasLength);