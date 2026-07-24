
#include "moog_filter.h"

inline static float fast_tanh(float x)
{
    const float x2 = x * x;
    return x * (27.0 + x2) / (27.0 + 9.0 * x2);
}

static inline void calculateCutoff(MicrotrackerMoog *mm)
{
    mm->coCalc = mm->cutoff + mm->cutmod;
    mm->coCalc = mm->coCalc * 44100 / mm->sampleRate; // 6.28318530717 * 1000 / sampleRate;
    mm->coCalc = mm->coCalc > 1 ? 1 : (mm->coCalc < 0 ? 0 : mm->coCalc);
}

void init_MicrotrackerMoog(MicrotrackerMoog *mm, float sampleRate)
{
    mm->sampleRate = sampleRate;

    mm->p0 = mm->p1 = mm->p2 = mm->p3 = mm->p32 = mm->p33 = mm->p34 = 0.0;
    mm->cutmod = 0;
    MicrotrackerMoog_setCutoff(mm, 1.0f);
    MicrotrackerMoog_setResonance(mm, 0.10f);
}

float MicrotrackerMoog_calculate(MicrotrackerMoog *mm, float x)
{
    const float k = 4 * mm->resonance;
    // Coefficients optimized using differential evolution
    // to make feedback gain 4.0 correspond closely to the
    // border of instability, for all values of omega.
    const float out = mm->p3 * 0.360891 + mm->p32 * 0.417290 + mm->p33 * 0.177896 + mm->p34 * 0.0439725;

    mm->p34 = mm->p33;
    mm->p33 = mm->p32;
    mm->p32 = mm->p3;

    mm->p0 += (fast_tanh(x - k * out) - fast_tanh(mm->p0)) * mm->coCalc;
    mm->p1 += (fast_tanh(mm->p0) - fast_tanh(mm->p1)) * mm->coCalc;
    mm->p2 += (fast_tanh(mm->p1) - fast_tanh(mm->p2)) * mm->coCalc;
    mm->p3 += (fast_tanh(mm->p2) - fast_tanh(mm->p3)) * mm->coCalc;

    return out;
}

void MicrotrackerMoog_setResonance(MicrotrackerMoog *mm, float r)
{
    mm->resonance = r;
}

void MicrotrackerMoog_setCutoff(MicrotrackerMoog *mm, float c)
{
    mm->cutoff = c;
    calculateCutoff(mm);
}

inline void MicrotrackerMoog_setModulation(MicrotrackerMoog *mm, float m)

{
    if (mm->cutmod != m)
    {
        mm->cutmod = m;
        calculateCutoff(mm);
    }
}

void MicrotrackerMoog_reset(MicrotrackerMoog *mm)
{
    mm->p0 = mm->p1 = mm->p2 = mm->p3 = mm->p32 = mm->p33 = mm->p34 = 0.0;
}

void MicrotrackerMoog_setSamplerate(MicrotrackerMoog *mm, int sr)
{
    mm->sampleRate = sr;
    calculateCutoff(mm);
}