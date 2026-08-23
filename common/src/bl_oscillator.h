#pragma once

// Band-limited version of simple_oscillator

// Oscillator waveform types
enum OscType
{
    OSC_TRIANGLE,
    OSC_SAW,
    OSC_SQUARE
};

/*
 * An oscillator for music syntheziser use. Has the following waveforms:
 * triangle wave, saw wave, square wave
 */
typedef struct
{
    // Current phase (0...1)
    float phase;
    // Phase increment: this is added to the phase on each sample
    float frequency;
    unsigned char note_idx_saw, note_idx_sqr; 
} BlOscillator;

/*
 * Proceeds the state by one sample. Get the signal amplitude value using the BlOscillator_getValue function.
 */
void BlOscillator_calculateNext(BlOscillator * bo);
/*
 * Get current oscillator signal value for the given oscillator type.
 */
float BlOscillator_getValue(BlOscillator * bo, enum OscType oscType);

void BlOscillator_setFrequency(BlOscillator * bo, float inc, unsigned char note);
