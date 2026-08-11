#include "2opfmsynth.h"
#include "float_math.h"
#include <string.h>
#include <math.h>

static void increment_oscillator(Oscillator *osc)
{
    osc->phase += osc->phinc;
    if (osc->phase > 1)
        osc->phase -= 1;
}

void reset_voice(VoiceFmSynth2Op *synth)
{
    synth->modulation_amount = synth->original_modulation_amount;
    synth->modulation_env = synth->original_modulation_env;
    synth->osc1.phase = 0;
    synth->osc2.phase = 0;
}

void set_note_inc(VoiceFmSynth2Op *synth, float inc)
{
    synth->osc1.phinc = inc;
    synth->osc2.phinc = synth->osc1.phinc * synth->freq_ratio;
}

static inline float _sin_with_asym_clip(float phase, float asym_clip)
{
    const float o = fastsinfullf(phase);
    if (o > asym_clip)
        return asym_clip;
    return o;
}

float process_voice_fm_synth_2op(VoiceFmSynth2Op *synth, float asym_clip)
{
    synth->modulation_amount *= synth->modulation_env;
    if (synth->modulation_amount > 20)
    {
        synth->modulation_amount = 20;
        synth->modulation_env = 1 / synth->modulation_env;
    }
    else if (synth->modulation_amount < 1e-9)
        synth->modulation_amount = 0;
    const float mod =
        _sin_with_asym_clip(synth->osc2.phase * 3.14159265358979323846f * 2, asym_clip) * synth->modulation_amount;
    increment_oscillator(&synth->osc1);
    increment_oscillator(&synth->osc2);
    return _sin_with_asym_clip(synth->osc1.phase * 3.14159265358979323846f * 2 + mod, asym_clip);
}
