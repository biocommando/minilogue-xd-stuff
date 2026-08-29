#pragma once
#include <stdint.h>

// Based on
// https://github.com/ddiakopoulos/MoogLadders/blob/master/src/MicrotrackerModel.h
// which is based on implementation by Magnus Jonsson
// https://github.com/magnusjonsson/microtracker (unlicense)

// Filter state. Internal state not intended to be read directly.
typedef struct
{
    int32_t p0;
    int32_t p1;
    int32_t p2;
    int32_t p3;
    int32_t p32;
    int32_t p33;
    int32_t p34;
    int32_t cutoff, cutmod, coCalc;
    int32_t resonance;
    int32_t sampleRate;
} MicrotrackerMoog;

/*
 * Initializes the filter structure with the sample rate (Hz).
 */
void init_MicrotrackerMoog(MicrotrackerMoog *mm, float sampleRate);

/*
 * Proceeds the filter effect state by one sample. The input signal should be
 * fed to this function sample by sample and the output signal is returned from
 * this function sample by sample.
 */
int16_t MicrotrackerMoog_calculate(MicrotrackerMoog *mm, int16_t x);

/*
 * Set the resonance parameter (range 0-1).
 */
void MicrotrackerMoog_setResonance(MicrotrackerMoog *mm, float r);

/*
 * Set the filter cutoff parameter (range 0-1).
 */
void MicrotrackerMoog_setCutoff(MicrotrackerMoog *mm, float c);

/*
 * Set cutoff modulation value. The modulation value is added to cutoff parameter when calculating
 * the cutoff coefficient.
 */
void MicrotrackerMoog_setModulation(MicrotrackerMoog *mm, float m);

/*
 * Resets the filter delay line to 0.
 */
void MicrotrackerMoog_reset(MicrotrackerMoog *mm);

/*
 * Sets the sample rate in Hz.
 */
void MicrotrackerMoog_setSamplerate(MicrotrackerMoog *mm, int sr);
