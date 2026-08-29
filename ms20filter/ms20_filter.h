#pragma once

typedef struct {
    float k2, T;
    float C1, C2, R1, R2;
    float xm1, xm2, ym1, ym2;
    float Ax, Bx, Cx, Ay, By, Cy;
    float iCy, Cx0, tempTerm1;
    float cutoff, integratedCutoff, resonance, cutmod, pC1, pR1;
    int sampleIdx;
} MS20Filter;

void MS20Filter_init(MS20Filter *f, int samplfreq);
float MS20Filter_calculate(MS20Filter *f, float x);
void MS20Filter_setCutoff(MS20Filter *f, float v);
void MS20Filter_setResonance(MS20Filter *f, float v);
void MS20Filter_setModulation(MS20Filter *f, float v);
void MS20Filter_setComponentValues(MS20Filter *f, float c, float r);
void MS20Filter_setSamplerate(MS20Filter *f, int rate);
void MS20Filter_reset(MS20Filter *f);
