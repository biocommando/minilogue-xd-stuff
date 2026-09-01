#include "ms20_filter.h"
#include "usermodfx.h"

static inline void update(MS20Filter *f)
{
	f->k2 = 2.2f * f->resonance;
	const float cmtemp = 10.0f - (f->integratedCutoff * (f->cutmod + 1) * 1.4f);
	f->R1 = fasterpow2f(cmtemp * 0.7382f) * f->pR1;
	f->R2 = 3 * f->R1;
	f->Cx = f->Cx0 / f->R1;
	f->Bx = 2 * f->Cx;
	f->Ax = f->Cx;
	const float tempTerm2 = (2 * f->k2 * f->T) / f->C2;        // dependent on resonance
	const float tempTerm3 = (2 * f->R2 * f->T) / (f->C1 * f->R1); // dependent on cutoff
	f->Cy = 4 * f->R2 + f->tempTerm1 - tempTerm2 + tempTerm3 + f->Cx;
	f->By = ((-8) * f->R2 + f->Bx);
	f->Ay = 4 * f->R2 - f->tempTerm1 + tempTerm2 - tempTerm3 + f->Ax;
	f->iCy = 1 / f->Cy;
}

static inline void updateConstants(MS20Filter *f)
{
	f->C1 = f->pC1;
	f->C2 = f->C1 * 0.30303f;
	f->Cx0 = (f->T * f->T) / (f->C1 * f->C2);
	f->tempTerm1 = (2 * f->T) / f->C1 + (2 * f->T) / f->C2;
}

void MS20Filter_init(MS20Filter *f, int samplfreq)
{
	f->T = 1.0f / samplfreq;
	f->pC1 = 0.0000000033f;
	f->pR1 = 20000;
	f->xm2 = 0;
	f->xm1 = 0;
	f->ym1 = 0;
	f->ym2 = 0;
	f->sampleIdx = 14;
	MS20Filter_setCutoff(f, 1);
	f->integratedCutoff = f->cutoff;
	MS20Filter_setResonance(f, 0);
	MS20Filter_setModulation(f, 0);
	updateConstants(f);
	update(f);
}

float MS20Filter_calculate(MS20Filter *f, float x)
{
	f->integratedCutoff = f->integratedCutoff + (f->cutoff - f->integratedCutoff) * 0.01f;
	if (++(f->sampleIdx) == 15)
	{
		f->sampleIdx = 0;
		update(f);
	}
	const float y = (f->Ax * f->xm2 + f->Bx * f->xm1 + f->Cx * x - f->Ay * f->ym2 - f->By * f->ym1) * f->iCy;

	// saturation
	if (x < -1)
		x = -1;
	else if (x > 1)
		x = 1;
	f->xm2 = f->xm1;
	f->xm1 = x;
	f->ym2 = f->ym1;
	f->ym1 = y;
	return y;
}
void MS20Filter_setCutoff(MS20Filter *f, float v)
{
	f->cutoff = 10 * v;
}

void MS20Filter_setResonance(MS20Filter *f, float v)
{
	f->resonance = v;
}

void MS20Filter_setModulation(MS20Filter *f, float v)
{
	f->cutmod = v;
}

void MS20Filter_setComponentValues(MS20Filter *f, float c, float r)
{
	f->pC1 = c;
	f->pR1 = r;
	updateConstants(f);
}

void MS20Filter_setSamplerate(MS20Filter *f, int rate)
{
	f->T = 1.0f / (float)rate;
	updateConstants(f);
	update(f);
}
void MS20Filter_reset(MS20Filter *f)
{
	f->integratedCutoff = f->cutoff;
	f->xm2 = 0;
	f->xm1 = 0;
	f->ym1 = 0;
	f->ym2 = 0;
	f->sampleIdx = 14;
	updateConstants(f);
	update(f);
}
