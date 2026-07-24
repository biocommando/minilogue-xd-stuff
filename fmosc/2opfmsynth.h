#pragma once
/*
 * Oscillator with a phase and phase increment.
 */
typedef struct {
    float phase, phinc;
} Oscillator;

/*
 * FM Synth with 2 operators.
 * The voice architecture is:
 * op2 -> op1 -> output
 * A decay envelope can be applied to both output volume and
 * the op2 -> op1 modulation amount.
 * The frequency of op2 is the frequency of op1 multiplied by the
 * freq_ratio.
 */
typedef struct {
    Oscillator osc1;
    Oscillator osc2;
    float freq_ratio;
    float modulation_amount, original_modulation_amount;
    float modulation_env, original_modulation_env;
} VoiceFmSynth2Op;

void reset_voice(VoiceFmSynth2Op *synth);
void set_note_inc(VoiceFmSynth2Op *synth, float inc);

/*
 * Progresses the synth voice by one sample and returns the current sample value.
 */
float process_voice_fm_synth_2op(VoiceFmSynth2Op *synth, float asym_clip);
