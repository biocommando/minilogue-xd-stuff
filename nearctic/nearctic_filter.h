#pragma once

typedef struct
{
    float filterMemory[7];
    float cutoff;
    float resonance;
    float cutoffModulation;
    float sampleRate;

    /*
     Variable names relate to filter architecture that consists of
     two lowpass filter blocks (LA and LB), three saturation
     blocks (SA, SB, SC) and a highpass block (HA)
    */

    float cutoffCoeffLA;
    float cutoffCoeffHA;
    float cutoffCoeffLB;
} NearcticFilter;

void NearcticFilter_setCutoff(NearcticFilter *f, float value);
void NearcticFilter_setModulation(NearcticFilter *f, float value);
void NearcticFilter_setResonance(NearcticFilter *f, float value);
void NearcticFilter_reset(NearcticFilter *f);
float NearcticFilter_calculate(NearcticFilter *f, float input);
void NearcticFilter_setSamplerate(NearcticFilter *f, int newSampleRate);
void NearcticFilter_init(NearcticFilter *f, float sampleRate);
