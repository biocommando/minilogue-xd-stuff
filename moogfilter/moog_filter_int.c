#include "moog_filter_int.h"
#include "usermodfx.h"

#define coeff_bit_width 10 // 16 is too much, 12 was still ok
#define coeff_div (1 << coeff_bit_width)
#define float_conv_coeff(f) (int32_t)(f * coeff_div)
#define coeff_mult(expr) ((expr) >> coeff_bit_width)

static __sdram int16_t cubicsat_tbl[65536];

#define CUBIC_SAT(input) \
    ((input) - (input) * (input) * (input) / 3.0f)

static void init_cubicsat_tbl()
{
    for (int32_t i = INT16_MIN; i <= INT16_MAX; i++)
    {
        float v = i / (float)0x7fff;
        v = CUBIC_SAT(v);
        const int16_t ii = i;
        cubicsat_tbl[(uint16_t)ii] = v * 0x7fff;
    }
}

inline static int32_t fast_tanh(int32_t x)
{
    if (x > INT16_MAX)
        return INT16_MAX;
    if (x < INT16_MIN)
        return INT16_MIN;
    return cubicsat_tbl[(uint16_t)x];
}

/**
 * Taylor square root optimized for inputs between 0.0 and 1.0.
 *
 * Square root is calculated only once when samplerate changes but using
 * sqrtf from math.h pulls in ~2kB of machine code so let's still use an
 * approximation.
 */
static inline float taylor_sqrt_0_to_1(float x) {
    const float d = x - 0.25f;

    // 1st-order: d / (2 * 0.5) = d
    const float linear_term = d;

    // 2nd-order: -(d^2) / (8 * 0.5^3) = -(d^2) / 1 = -d^2
    const float quadratic_term = -(d * d);

    // Result = 0.5 + d - d^2
    return 0.5f + linear_term + quadratic_term;
}

static float getSampleRateCutoffRatio(MicrotrackerMoog *mm)
{
    const float sr_ratio = 44100.0f / mm->sampleRate;
    return taylor_sqrt_0_to_1(sr_ratio);
}


static inline void calculateCutoff(MicrotrackerMoog *mm)
{
    mm->coCalc = mm->cutoff + mm->cutmod;
    mm->coCalc = mm->coCalc * getSampleRateCutoffRatio(mm); // 6.28318530717 * 1000 / sampleRate;
    if (mm->coCalc > INT16_MAX) mm->coCalc = INT16_MAX;
    mm->coCalc = mm->coCalc < 0 ? 0 : mm->coCalc;
}

void init_MicrotrackerMoog(MicrotrackerMoog *mm, float sampleRate)
{
    static uint8_t tbl_init = 0;
    if (!tbl_init)
    {
        tbl_init = 1;
        init_cubicsat_tbl();
    }
    mm->sampleRate = sampleRate;

    mm->p0 = mm->p1 = mm->p2 = mm->p3 = mm->p32 = mm->p33 = mm->p34 = 0.0;
    mm->cutmod = 0;
    MicrotrackerMoog_setCutoff(mm, 1.0f);
    MicrotrackerMoog_setResonance(mm, 0.10f);
}

static const int32_t p3f = float_conv_coeff(0.360891);
static const int32_t p32f = float_conv_coeff(0.417290);
static const int32_t p33f = float_conv_coeff(0.177896);
static const int32_t p34f = float_conv_coeff(0.0439725);

int16_t MicrotrackerMoog_calculate(MicrotrackerMoog *mm, int16_t x)
{
    const int k = 4 * mm->resonance;
    // Coefficients optimized using differential evolution
    // to make feedback gain 4.0 correspond closely to the
    // border of instability, for all values of omega.
    int32_t out = coeff_mult(mm->p3 * p3f) + coeff_mult(mm->p32 * p32f) + coeff_mult(mm->p33 * p33f) + coeff_mult(mm->p34 * p34f);

    mm->p34 = mm->p33;
    mm->p33 = mm->p32;
    mm->p32 = mm->p3;

    mm->p0 += coeff_mult((fast_tanh(x - coeff_mult(k * out)) - fast_tanh(mm->p0)) * mm->coCalc);
    mm->p1 += coeff_mult((fast_tanh(mm->p0) - fast_tanh(mm->p1)) * mm->coCalc);
    mm->p2 += coeff_mult((fast_tanh(mm->p1) - fast_tanh(mm->p2)) * mm->coCalc);
    mm->p3 += coeff_mult((fast_tanh(mm->p2) - fast_tanh(mm->p3)) * mm->coCalc);

    return (int16_t)fast_tanh(out);
}

void MicrotrackerMoog_setResonance(MicrotrackerMoog *mm, float r)
{
    mm->resonance = float_conv_coeff(r);
}

void MicrotrackerMoog_setCutoff(MicrotrackerMoog *mm, float c)
{
    mm->cutoff = float_conv_coeff(c);
    calculateCutoff(mm);
}

inline void MicrotrackerMoog_setModulation(MicrotrackerMoog *mm, float m)

{
    if (mm->cutmod != m)
    {
        mm->cutmod = float_conv_coeff(m);
        calculateCutoff(mm);
    }
}

void MicrotrackerMoog_reset(MicrotrackerMoog *mm)
{
    mm->p0 = mm->p1 = mm->p2 = mm->p3 = mm->p32 = mm->p33 = mm->p34 = 0;
}

void MicrotrackerMoog_setSamplerate(MicrotrackerMoog *mm, int sr)
{
    mm->sampleRate = sr;
    calculateCutoff(mm);
}
