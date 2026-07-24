#include "adsr_envelope.h"

static void triggerStage(AdsrEnvelope *env, int stage)
{
    env->stage = stage;
    EnvelopeStage_reset(&env->stages[stage]);
}

float AdsrEnvelope_getSustain(AdsrEnvelope *env) { return env->sustain; }

void AdsrEnvelope_setCycleOnOff(AdsrEnvelope *env, int on) { env->cycleAttackDecay = on; }

int AdsrEnvelope_getReleaseStage(AdsrEnvelope *env) { return env->releaseStage; }

void init_AdsrEnvelope(AdsrEnvelope *env)
{
    env->cycleAttackDecay = 0;
    env->endReached = 1;
    env->sustain = 0;
    env->stage = 0;
    env->envelope = 0;
    init_EnvelopeStage(&env->stages[0], 1);
    init_EnvelopeStage(&env->stages[1], 1);
    init_EnvelopeStage(&env->stages[2], 0);
    init_EnvelopeStage(&env->stages[3], 1);
}

void AdsrEnvelope_setAttack(AdsrEnvelope *env, int samples)
{
    EnvelopeStage_setLength(&env->stages[0], samples);
}

void AdsrEnvelope_setDecay(AdsrEnvelope *env, int samples)
{
    EnvelopeStage_setLength(&env->stages[1], samples);
}

void AdsrEnvelope_setSustain(AdsrEnvelope *env, float level)
{
    env->sustain = level;
}

void AdsrEnvelope_setRelease(AdsrEnvelope *env, int samples)
{
    if (samples < 40)
        samples = 40; // to smoothen the clicking sound when using minimal release
    EnvelopeStage_setLength(&env->stages[3], samples);
}

void AdsrEnvelope_calculateNext(AdsrEnvelope *env)
{
    if (env->endReached)
    {
        return;
    }
    EnvelopeStage *current = &env->stages[env->stage];
    if (EnvelopeStage_hasNext(current))
    {
        EnvelopeStage_calcuateNext(current);
        float ratio = EnvelopeStage_getRatio(current);
        switch (env->stage)
        {
        case 0:
            env->envelope = ratio;
            break;
        case 1:
            env->envelope = 1 - (1 - env->sustain) * ratio;
            break;
        case 2:
            env->envelope = env->sustain;
            break;
        case 3:
            env->envelope = env->releaseLevel * (1 - ratio);
            break;
        default:
            break;
        }
    }
    else if (env->stage < 3)
    {
        // If cycling A-D-S-A-D-S... at the end of decay stage, trigger attack again
        if (env->cycleAttackDecay && env->stage == 1)
            triggerStage(env, 0);
        else if (env->stage == 1 && env->sustain == 0)
        {
            // Special case: decay ends and sustain is zero
            env->envelope = 0;
            env->endReached = 1;
            env->stages[env->stage].ratio = 1;
        }
        else
        {
            triggerStage(env, env->stage + 1);
        }
        AdsrEnvelope_calculateNext(env);
    }
    else
    {
        // release ended
        env->envelope = 0;
        env->endReached = 1;
    }
}

int AdsrEnvelope_ended(AdsrEnvelope *env)
{
    return env->endReached;
}

void AdsrEnvelope_trigger(AdsrEnvelope *env)
{
    env->endReached = 0;
    triggerStage(env, 0);
}

void AdsrEnvelope_release(AdsrEnvelope *env)
{
    if (env->stage < 3 && !env->endReached)
    {
        env->releaseLevel = env->envelope;
        env->releaseStage = env->stage;
        triggerStage(env, 3);
    }
}

int AdsrEnvelope_getStage(AdsrEnvelope *env)
{
    return env->stage;
}

float AdsrEnvelope_getRatio(AdsrEnvelope *env, int xStage)
{
    xStage = xStage == -1 ? env->stage : xStage;
    return EnvelopeStage_getRatio(&env->stages[xStage]);
}

float AdsrEnvelope_getEnvelope(AdsrEnvelope *env)
{
    return env->envelope;
}
