#pragma once

#include "envelope_stage.h"

// Ported from C++ code

/*
 * A linear attack-decay-sustain-release envelope.
 * Not dependent on sample rate as all stages are set in samples.
 */
typedef struct
{
    // Stages in order attack-decay-sustain-release
    EnvelopeStage stages[4];
    // Current stage index
    int stage;
    // Stage index at which the envelope release was triggered;
    // used for calculating the correct release envelope in case
    // where release was triggered before reaching the sustain stage.
    int releaseStage;
    // Has release stage ended?
    int endReached;
    // Set this to 1 to trigger attack stage again after decay instead
    // of entering sustain mode.
    int cycleAttackDecay;
    // Sustain level (envelope stages are just basically time counters so
    // they don't hold amplitude information).
    float sustain;
    // The envelope amplitude when release triggered.
    float releaseLevel;
    // The current envelope amplitude. Updated using AdsrEnvelope_calculateNext.
    float envelope;
} AdsrEnvelope;

// Set attack stage length in samples
void AdsrEnvelope_setAttack(AdsrEnvelope *env, int samples);
// Set decay stage length in samples
void AdsrEnvelope_setDecay(AdsrEnvelope *env, int samples);
// Set sustain level. The level should be between 0 and 1.
void AdsrEnvelope_setSustain(AdsrEnvelope *env, float level);
// Get the sustain level
float AdsrEnvelope_getSustain(AdsrEnvelope *env);
// Set release stage length in samples
void AdsrEnvelope_setRelease(AdsrEnvelope *env, int samples);
// Set cycleAttackDecay property
void AdsrEnvelope_setCycleOnOff(AdsrEnvelope *env, int on);

/*
 * Proceeds the envelope state by one sample. Modifies the envelope properties:
 * stage, endReached, envelope
 */
void AdsrEnvelope_calculateNext(AdsrEnvelope *env);
// Start the envelope (=triggers the attack stage)
void AdsrEnvelope_trigger(AdsrEnvelope *env);
// Release the envelope. This triggers the release stage and the envelope ends
// after the stage has been finished.
void AdsrEnvelope_release(AdsrEnvelope *env);

// Getter for stage
int AdsrEnvelope_getStage(AdsrEnvelope *env);
// Getter for releaseStage
int AdsrEnvelope_getReleaseStage(AdsrEnvelope *env);
// Get the finished ratio of the current envelope stage.
// E.g. 0.5 means that the envelope stage is half way finished.
float AdsrEnvelope_getRatio(AdsrEnvelope *env, int stage);
// Getter for envelope
float AdsrEnvelope_getEnvelope(AdsrEnvelope *env);
// Getter for endReached
int AdsrEnvelope_ended(AdsrEnvelope *env);

// Initializes the AdsrEnvelope structure
void init_AdsrEnvelope(AdsrEnvelope *env);