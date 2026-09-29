#include "usermodfx.h"
#include "ms20_filter.h"
#include "adsr_envelope.h"

static MS20Filter filter;

static AdsrEnvelope env;
static inline void auto_env(const float input)
{
    static uint16_t silence_counter = 0xffff;
    if (input < 0.05f && input > -0.05f)
    {
        if (silence_counter < 0xffff)
            silence_counter++;
    }
    else
    {
        if (silence_counter >= 480)
            AdsrEnvelope_trigger(&env);
        silence_counter = 0;
    }
    AdsrEnvelope_calculateNext(&env);
    MS20Filter_setModulation(&filter, AdsrEnvelope_getEnvelope(&env));
}

void MODFX_INIT(uint32_t platform, uint32_t api)
{
    (void) platform;
    (void) api;
    MS20Filter_init(&filter, 48000);
    init_AdsrEnvelope(&env);
}

void MODFX_PROCESS(const float *main_xn, float *main_yn, const float *sub_xn, float *sub_yn, uint32_t frames)
{
    (void) sub_xn;
    (void) sub_yn;

    const float *__restrict x = (const float *) main_xn;
    float *__restrict y = (float *) main_yn;

    for (uint32_t i = 0; i < frames; i++)
    {
        const float input = *x;
#ifdef AUTO_ENV
        auto_env(input);
#endif
        const float output = MS20Filter_calculate(&filter, input);
        *(y++) = output;
        *(y++) = output;
        x += 2;
    }
}

void MODFX_PARAM(uint8_t index, int32_t value)
{
    const float v = q31_to_f32(value);
    if (index == k_user_modfx_param_time)
    {
        MS20Filter_setResonance(&filter, v);
        AdsrEnvelope_setDecay(&env, 48000 * v);
    }
    else if (index == k_user_modfx_param_depth)
    {
        MS20Filter_setCutoff(&filter, v);
        AdsrEnvelope_setSustain(&env, v * 0.5f);
    }
}
