#include "manifest_params.h"
#include "adsr_envelope.h"
#include "basic_oscillator.h"
#include "userosc.h"
#include "synth_random.h"
#include "flt.h"
#include "stuff_util.h"

#define ENV_LENGTH_RATIO 1.3f
#define BASELINE_ATTACK_LENGTH_SEC 1.35f
static struct filter_state tone_filter, noise_filter;
static AdsrEnvelope env_a, env_d, env_noise;
static float overall_env_val = 1, overall_env = 1;
#define N_OSC 2
static BasicOscillator osc[N_OSC];

static float noise_mix, tone, dist_gain;
static uint8_t osc1_interval;

static const float invert_threshold = 17;
static const float clip_threshold = 0.95;

static const float intervals[] = {1.5, 2};

#define guitar_waveform_length 134
static const float guitar_waveform[guitar_waveform_length] = {0.04466445371508598, 0.13951027393341064, 0.24342605471611023, 0.3452039062976837,
  0.4491196870803833, 0.5418599843978882, 0.6253683567047119, 0.6849709153175354, 0.7290573716163635, 0.7494971752166748,
  0.7502098083496094, 0.7363133430480957, 0.712569534778595, 0.6803711652755737,
  0.6564977765083313, 0.6401718258857727, 0.6395887732505798, 0.6581173539161682, 0.6856511831283569, 0.7204409241676331,
  0.7481042742729187, 0.7526716589927673, 0.7396174073219299, 0.7003898620605469, 0.6538415551185608, 0.6058355569839478,
  0.5666080117225647, 0.5475934743881226, 0.5550438165664673,
  0.5940769910812378, 0.6539387106895447, 0.7299968004226685, 0.8056661486625671, 0.8806552290916443, 0.9405493140220642,
  0.9816880226135254, 0.997074544429779, 0.9806838631629944, 0.9356580376625061, 0.8707754611968994, 0.7885951399803162,
  0.7005842328071594, 0.6235543489456177, 0.5509623289108276,
  0.5019521713256836, 0.46210917830467224, 0.4358386993408203, 0.42093804478645325, 0.4040614664554596, 0.38281184434890747,
  0.3396647572517395, 0.28009459376335144, 0.19804388284683228, 0.10873720794916153, 0.011947828345000744, -0.08318952471017838,
  -0.18205204606056213, -0.2789385914802551, -0.37235912680625916,
  -0.4581349790096283, -0.5368168354034424, -0.5967109203338623, -0.6526207327842712, -0.6961565613746643, -0.7350277900695801,
  -0.7669345736503601, -0.793917715549469, -0.8180502653121948, -0.8379069566726685, -0.8575369715690613, -0.8625254034996033,
  -0.8608410358428955, -0.8545244336128235, -0.8389111757278442,
  -0.819993793964386, -0.804736852645874, -0.7869208455085754, -0.7829689383506775, -0.7729595899581909, -0.7633066177368164,
  -0.7410204410552979, -0.7035744786262512, -0.6418987512588501, -0.5607550740242004, -0.46011093258857727, -0.3476758897304535,
  -0.22552303969860077, -0.10466588288545609, 0.01904182881116867,
  0.1256462037563324, 0.22353693842887878, 0.29580506682395935, 0.3455602526664734, 0.3612706959247589, 0.3473418354988098,
  0.30623549222946167, 0.24938631057739258, 0.1856374740600586, 0.1236378476023674, 0.07537273317575455, 0.04204064607620239,
  0.02645975723862648, 0.01797286979854107, 0.01680673286318779,
  0.005793216172605753, -0.014614184387028217, -0.06080617010593414, -0.1275351345539093, -0.21444474160671234,
  -0.31337201595306396, -0.426131010055542, -0.5409307479858398, -0.653139054775238, -0.7606179714202881, -0.8491472601890564,
  -0.9264686107635498, -0.9733084440231323, -1.0, -0.999967634677887,
  -0.9796574115753174, -0.9366723299026489, -0.872437596321106, -0.7959908246994019, -0.7051941156387329, -0.6115143895149231,
  -0.5186445713043213, -0.4329334795475006, -0.3598555326461792, -0.2964954376220703, -0.2506273686885834, -0.19941452145576477,
  -0.15157051384449005, -0.07839540392160416, -0.0013655700022354722,};

void OSC_INIT(uint32_t platform, uint32_t api)
{
    (void) platform;
    (void) api;
    init_AdsrEnvelope(&env_a);
    init_AdsrEnvelope(&env_d);
    init_AdsrEnvelope(&env_noise);
    init_filter(&noise_filter, 1000, k_samplerate);
    for (int i = 0; i < N_OSC; i++)
    {
      init_BasicOscillator(&osc[i], k_samplerate);
      BasicOscillator_setWavetable(&osc[i], (float*)guitar_waveform, guitar_waveform_length);
      BasicOscillator_setWaveTableParams(&osc[i], 0, 1);
    }
}

static inline void update_inc(const user_osc_param_t *const params)
{
    const float inc = osc_w0f_for_note((params->pitch) >> 8, params->pitch & 0xFF);
    BasicOscillator_setFrequency(&osc[0], inc);
    BasicOscillator_setFrequency(&osc[1], inc * intervals[osc1_interval]);
}

void OSC_CYCLE(const user_osc_param_t *const params, int32_t *yn, const uint32_t frames)
{

    update_inc(params);

    const float shape_lfo = q31_to_f32(params->shape_lfo);

    OSC_LOOP(y, yn, frames)
    {
        AdsrEnvelope_calculateNext(&env_a);
        AdsrEnvelope_calculateNext(&env_d);
        AdsrEnvelope_calculateNext(&env_noise);
        BasicOscillator_calculateNext(&osc[0]);
        BasicOscillator_calculateNext(&osc[1]);

        float tri_out = BasicOscillator_getValue(&osc[0], OSC_TRIANGLE) + BasicOscillator_getValue(&osc[1], OSC_TRIANGLE);
        tri_out *= AdsrEnvelope_getEnvelope(&env_a) * 0.5;
        float wt_out = BasicOscillator_getValue(&osc[0], OSC_WT) + BasicOscillator_getValue(&osc[1], OSC_WT);
        wt_out *= AdsrEnvelope_getEnvelope(&env_d) * 0.5 + shape_lfo;
        float noise_out = (1 - (synth_random()&0xFFFFFF) / (float)0x7FFFFF);
        noise_out = process_filter(&noise_filter, noise_out);
        noise_out *= AdsrEnvelope_getEnvelope(&env_noise) * noise_mix;

        // tone bank
        float out = tri_out + wt_out + noise_out;
        if (tone > 0.5)
          out = process_hp_filter(&tone_filter, out);
        else
          out = process_filter(&tone_filter, out);

        // volume decay envelope
        out *= overall_env_val;
        overall_env_val *= overall_env;

        // distortion
        out *= dist_gain;
        if (out > invert_threshold || out < -invert_threshold)
            out *= -1;
        if (out > clip_threshold)
            out = clip_threshold;
        else if (out < -clip_threshold)
            out = -clip_threshold;

        *(y++) = safe_f32_to_q31(out);
    }
}

void OSC_NOTEON(const user_osc_param_t *const params)
{
    update_inc(params);
    overall_env_val = 1;
    AdsrEnvelope_trigger(&env_a);
    AdsrEnvelope_trigger(&env_d);
    AdsrEnvelope_trigger(&env_noise);
}

void OSC_NOTEOFF(const user_osc_param_t *const params)
{
    (void) params;
}


void OSC_PARAM(uint16_t index, uint16_t value)
{
    switch (index)
    {
        case USER_PARAM__Attack__idx:
            {
                const float v = value / 100.0;
                float attack_length = BASELINE_ATTACK_LENGTH_SEC * k_samplerate;
                attack_length = attack_length * (0.2 + v * 0.8);
                float decay_length = attack_length * ENV_LENGTH_RATIO;
                AdsrEnvelope_setAttack(&env_a, (int)attack_length);
                AdsrEnvelope_setSustain(&env_a, 1);
                AdsrEnvelope_setDecay(&env_d, (int)decay_length);
                decay_length = 2 * 0.015 * k_samplerate;
                decay_length = decay_length * (0.5 + v * 0.5);
                AdsrEnvelope_setDecay(&env_noise, (int)decay_length);
            }
            break;
        case USER_PARAM__Interval__idx:
            osc1_interval = value;
            break;
        case USER_PARAM__Noise_mix__idx:
            noise_mix = value / 100.0 * 0.5;
            break;
        case USER_PARAM__Decay__idx:
            {
                const float v = value / 100.0;
                overall_env = 1 - 0.0002 * v * v;
            }
            break;
        case k_user_osc_param_shape:
            {
                const float p_tone = tone;
                tone = param_val_to_f32(value);
                const float s0 = tone_filter.state0, s1 = tone_filter.state1;
                if (tone > 0.5)
                    init_hp_filter(&tone_filter, 10 + 2000 * 2 * (tone - 0.5), k_samplerate);
                else
                    init_filter(&tone_filter, 10000 - 9500 * 2 * (0.5 - tone), k_samplerate);
                if ((p_tone <= 0.5 && tone <= 0.5) || (p_tone > 0.5 && tone > 0.5))
                {
                    tone_filter.state0 = s0;
                    tone_filter.state1 = s1;
                }
            }
            break;
        case k_user_osc_param_shiftshape:
            dist_gain = 1 + 20 * param_val_to_f32(value);
            break;
        default:
            break;
    }
}
