#include "basic_oscillator.h"
#include "synth_random.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

static float sine_table[256];

static inline void init_sine_table()
{
    static int sine_table_initialized = 0;
    if (!sine_table_initialized)
    {
        sine_table_initialized = 1;
        for (int i = 0; i < 256; i++)
            sine_table[i] = sin(6.283185307179586476925286766559 / 256 * i);
    }
}

static inline void set_default_wt(BasicOscillator* bo)
{
    BasicOscillator_setWavetable(bo, sine_table, 1);
    BasicOscillator_setWaveTableParams(bo, 0, 1);
}

void init_BasicOscillator(BasicOscillator* bo, int sampleRate)
{
    memset(bo, 0, sizeof(BasicOscillator));
    BasicOscillator_setSamplerate(bo, sampleRate);
    init_sine_table();
    set_default_wt(bo);
}

void BasicOscillator_setWaveTableParams(BasicOscillator* bo, float pos, float window)
{
    bo->wtWindow = 2 + window * (bo->wt_size - 2);
    bo->wtPos = pos * (bo->wt_size - bo->wtWindow - 1);
}

void BasicOscillator_setWavetable(BasicOscillator* this, float *wt, int size)
{
    this->wt = wt;
    this->wt_size = size;
}


inline void BasicOscillator_setSamplerate(BasicOscillator* bo, int rate)
{
    bo->hzToF = 1.0f / (float)rate;
}

void BasicOscillator_randomizePhase(BasicOscillator* bo, float rndAmount)
{
    bo->phase = (float)(synth_random() % 100000) * 0.00001f;
    bo->phase *= rndAmount;
}

inline static float sin1(float phase)
{
    return sine_table[(int)(phase * 256)];
}
inline static float tri1(float phase)
{
    if (phase < 0.5)
        return 4 * phase - 1;
    else
        return -4 * phase + 3;
}
inline static float saw1(float phase)
{
    return 2 * phase - 1;
}
inline static float sqr1(float phase)
{
    return phase < 0.5 ? 1.0f : -1.0f;
}

inline static float wt1(float *wt, float phase, int pos, int window, float *dcFilterState)
{
    const float value = wt[(int)(pos + window * phase)];

    // Remove DC offset
    dcFilterState[0] = 0.9984 * (value + dcFilterState[0] - dcFilterState[1]);
    dcFilterState[1] = value;

    return dcFilterState[0];
}

void BasicOscillator_calculateNext(BasicOscillator* bo)
{
    bo->phase = bo->phase + bo->frequency;
    if (bo->phase >= 1.0)
    {
        if (bo->wt_oneshot)
        {
            bo->wt_oneshot = 0;
            set_default_wt(bo);
        }
        bo->phase = bo->phase - 1.0f;
    }
}
void BasicOscillator_setFrequency(BasicOscillator* bo, float f_Hz)
{
    bo->frequency = f_Hz;// * bo->hzToF;
}

inline static float get_value(float p, enum OscType oscType, BasicOscillator *bo)
{
    switch (oscType)
    {
    case OSC_SINE:
        return sin1(p);
    case OSC_TRIANGLE:
        return tri1(p);
    case OSC_SAW:
        return saw1(p);
    case OSC_SQUARE:
        return sqr1(p);
    case OSC_WT:
        return wt1(bo->wt, p, bo->wtPos, bo->wtWindow, bo->dcFilterState);
    default:
        return 0;
    }
}

float BasicOscillator_getValue(BasicOscillator* bo, enum OscType oscType)
{
    return get_value(bo->phase, oscType, bo);
}

float BasicOscillator_getValueFm(BasicOscillator* bo, enum OscType oscType, float fmAmount)
{
    float p = bo->phase + fmAmount;
    while (p >= 1) p -= 1;
    while (p < 0) p += 1;

    return get_value(p, oscType, bo);
}
