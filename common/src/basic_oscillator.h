#pragma once

// Ported from C++ code

// Oscillator waveform types
enum OscType
{
    OSC_SINE,
    OSC_TRIANGLE,
    OSC_SAW,
    OSC_SQUARE,
    // WT = wavetable
    OSC_WT
};

/*
 * An oscillator for music syntheziser use. Has the following waveforms:
 * 256-point wavetable sine wave, triangle wave, saw wave, square wave, custom wavetable
 */
typedef struct
{
    // Current phase (0...1)
    float phase;
    // Phase increment: this is added to the phase on each sample
    float frequency;
    // The conversion factor from Hz to phase increment
    float hzToF;
    /*
     * Which part of the wavetable is played back (and looped) is determined by position and window.
     * Position determines the start index in wt array and window is the offset from the position
     * after which the playback loops back to start index. The current index can be calculated simply
     * by formula: pos + window * phase
     *
     * Use BasicOscillator_setWaveTableParams for setting up the parameters. Should be called again when
     * BasicOscillator_setWavetable is called.
     */
    int wtPos, wtWindow;
    /*
     * Use this flag to play wavetable waveform only once. This makes it possible to use the BasicOscillator
     * for sample playback e.g. in a drum machine. This flag is reset by the processing when the end of the
     * playback buffer is reached.
     */
    int wt_oneshot;

    /*
     * Pointer to the wavetable data. Note that this data must be managed by the calling code.
     * Points to the beginning of the sine wave wavetable so using OSC_WT waveform without calling
     * BasicOscillator_setWavetable just doesn't make any sound but doesn't crash the program either.
     */
    float *wt;
    /*
     * Size of the wavetable data.
     */
    int wt_size;

    /*
     * DC offset removal single order highpass filter state for wavetable oscillator output.
     */
    float dcFilterState[2];
} BasicOscillator;

/*
 * Initializes the structure with the sample rate (Hz).
 */
void init_BasicOscillator(BasicOscillator *bo, int sampleRate);
/*
 * Proceeds the state by one sample. Get the signal amplitude value using the BasicOscillator_getValue* functions.
 */
void BasicOscillator_calculateNext(BasicOscillator *bo);
/*
 * Set wavetable data. BasicOscillator_setWaveTableParams must be called after calling this.
 */
void BasicOscillator_setWavetable(BasicOscillator *bo, float *wt, int size);
/*
 * Get current oscillator signal value for the given oscillator type.
 */
float BasicOscillator_getValue(BasicOscillator *bo, enum OscType oscType);
/*
 * Same as getValue but applies a phase offset before returning the value. Allows using the oscillators for
 * FM synthesis or adding vibrato.
 */
float BasicOscillator_getValueFm(BasicOscillator *bo, enum OscType oscType, float fmAmount);
/*
 * Sets oscillator frequency as Hz.
 */
void BasicOscillator_setFrequency(BasicOscillator *bo, float f_Hz);
/*
 * Sets wavetable start position and playback length parameters. Parameters in range 0-1.
 * Position of 0 means the start of the buffer and 1 the end of the buffer.
 * Window of 0 means the same end position as start position and 1 that end position is
 * at the end of the buffer.
 */
void BasicOscillator_setWaveTableParams(BasicOscillator *bo, float pos, float window);
/*
 * Set sample rate in Hz.
 */
void BasicOscillator_setSamplerate(BasicOscillator *bo, int rate);

/*
 * Randomizes the phase. The amount parameter should be in range 0-1. The amount parameter
 * limits the initial phase to range 0-amount.
 */
void BasicOscillator_randomizePhase(BasicOscillator *bo, float rndAmount);