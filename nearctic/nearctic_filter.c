#include "nearctic_filter.h"

static inline float hardClip(float input)
{
    if (input > 1)
        return 1;
    if (input < -1)
        return -1;
    return input;
}

static inline float lpCoeff(NearcticFilter *f, float new_cutoff)
{
    const float cutFreqHz = new_cutoff * 0.5f * 44100;
    return 1.0f / (1.0f / (2.0f * 3.14159265f * 1.0f / f->sampleRate * cutFreqHz) + 1.0f);
}

static inline float hpCoeff(NearcticFilter *f, float new_cutoff)
{
    const float cutFreqHz = new_cutoff * 0.5f * 44100;
    return 1.0f / (2.0f * 3.14159265f * 1.0f / f->sampleRate * cutFreqHz + 1.0f);
}

static inline void updateCutoff(NearcticFilter *f)
{
    float modulatedCutoff = f->cutoff + f->cutoffModulation;
    modulatedCutoff = modulatedCutoff < 0 ? 0 : modulatedCutoff;
    const float cutoffLAHA = modulatedCutoff * 0.5f + 0.002f;
    const float cutoffLB = cutoffLAHA * 0.5f;

    f->cutoffCoeffLA = lpCoeff(f, cutoffLAHA);
    f->cutoffCoeffLB = lpCoeff(f, cutoffLB);
    f->cutoffCoeffHA = hpCoeff(f, cutoffLAHA);
}

inline static float calculateLpFilter(float *filter, float input, float cutoff)
{
    const float output = *filter + cutoff * (input - *filter);
    *filter = output;
    return output;
}

inline static float calculateHpFilter(float *filter, float input, float cutoff)
{
    const float output = cutoff * (input + filter[0] - filter[1]);
    filter[0] = output;
    filter[1] = input;
    return output;
}

void NearcticFilter_setCutoff(NearcticFilter *f, float value)
{
    f->cutoff = value;
    if (f->cutoff < 0)
        f->cutoff = 0;
    updateCutoff(f);
}

void NearcticFilter_setModulation(NearcticFilter *f, float value)
{
    f->cutoffModulation = hardClip(value);
    updateCutoff(f);
}

void NearcticFilter_setResonance(NearcticFilter *f, float value)
{
    f->resonance = value;
}

void NearcticFilter_reset(NearcticFilter *f)
{
    for (int i = 0; i < 7; i++)
    {
        f->filterMemory[i] = 0;
    }
    f->cutoff = 1;
    f->resonance = 0;
    f->cutoffCoeffHA = f->cutoffCoeffLA = f->cutoffCoeffLB = 0;
}

float NearcticFilter_calculate(NearcticFilter *f, float input)
{
    const int LA0 = 0, LA1 = 1, LB0 = 2, LB1 = 3, HA = 4, TOTAL = 6;

    const float LA0Input = f->filterMemory[TOTAL] + input;
    const float LA0Output = calculateLpFilter(&f->filterMemory[LA0], LA0Input, f->cutoffCoeffLA);
    const float LA1Input = LA0Output + f->filterMemory[TOTAL];
    const float LA1Output = calculateLpFilter(&f->filterMemory[LA1], LA1Input, f->cutoffCoeffLA);

    const float LB0Input = LA0Input;
    const float LB0Output = calculateLpFilter(&f->filterMemory[LB0], LB0Input, f->cutoffCoeffLB);
    const float LB1Input = LB0Output;
    const float LB1Output = calculateLpFilter(&f->filterMemory[LB1], LB1Input, f->cutoffCoeffLB);

    const float SAInput = LA1Output + LB1Output;
    const float SAOutput = hardClip(SAInput);

    const float HAInput = input + SAOutput;
    const float HAOutput = calculateHpFilter(&f->filterMemory[HA], HAInput, f->cutoffCoeffHA);

    const float SBInput = HAOutput;
    const float SBOutput = hardClip(SBInput);

    const float SCInput = SBOutput * f->resonance * 2;
    const float SCOutput = hardClip(SCInput);

    f->filterMemory[TOTAL] = SCOutput;

    return (HAOutput + SAOutput + LA1Output) * 0.33f;
}

void NearcticFilter_setSamplerate(NearcticFilter *f, int newSampleRate)
{
    f->sampleRate = newSampleRate;
    updateCutoff(f);
}

void NearcticFilter_init(NearcticFilter *f, float sampleRate)
{
    NearcticFilter_setSamplerate(f, sampleRate);
    NearcticFilter_reset(f);
}
