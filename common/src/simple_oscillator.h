#pragma once

// Stripped down version of basic_oscillator

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
} SimpleOscillator;

/*
 * Proceeds the state by one sample. Get the signal amplitude value using the SimpleOscillator_getValue function.
 */
void SimpleOscillator_calculateNext(SimpleOscillator * so);
/*
 * Get current oscillator signal value for the given oscillator type.
 */
float SimpleOscillator_getValue(SimpleOscillator * so, enum OscType oscType);
