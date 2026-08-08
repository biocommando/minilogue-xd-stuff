#pragma once

#include <stdint.h>

#include <math.h>
   typedef enum {
     k_user_osc_param_id1 = 0,
     k_user_osc_param_id2,
     k_user_osc_param_id3,
     k_user_osc_param_id4,
     k_user_osc_param_id5,
     k_user_osc_param_id6,
     k_user_osc_param_shape,
     k_user_osc_param_shiftshape,
     k_num_user_osc_param_id
   } user_osc_param_id_t;
   typedef struct user_osc_param {
     int32_t  shape_lfo;
     uint16_t pitch;
     uint16_t cutoff;
     uint16_t resonance;
     uint16_t reserved0[3];
   } user_osc_param_t;

   
static inline float clipmaxf(const float x, const float m)
 { return (((x)>=m)?m:(x)); }
static inline float linintf(const float fr, const float x0, const float x1) {
   return x0 + fr * (x1 - x0);
 }
 #define k_note_mod_fscale      (0.00392156862745098f)
 #define k_note_max_hz          (23679.643054f)
 #define k_samplerate        (48000)
 #define k_samplerate_recipf (2.08333333333333e-005f)
 

static inline float param_val_to_f32(uint16_t v)
{
    return v / 1023.0f;
}

typedef int32_t q31_t;


#define q15_to_f32_c 3.05175781250000e-005f
#define q31_to_f32_c 4.65661287307739e-010f

#define q15_to_f32(q) ((float)(q) * q15_to_f32_c)
#define q31_to_f32(q) ((float)(q) * q31_to_f32_c)

#define f32_to_q15(f)   ((q15_t)ssat((q31_t)((float)(f) * ((1<<15)-1)),16))
#define f32_to_q31(f)   ((q31_t)((float)(f) * (float)0x7FFFFFFF))

#define __fast_inline


static inline float midi_note_to_increment(int note)
{
    return (float)(440.0 * pow(2, (note - 69) / 12.0)) / k_samplerate * 2;
}
static inline float osc_w0f_for_note(uint8_t note, uint8_t mod)
{
    const float f0 = midi_note_to_increment(note);
    const float f1 = midi_note_to_increment(note + 1);

    const float f = clipmaxf(linintf(mod * k_note_mod_fscale, f0, f1), k_note_max_hz);

    return f; // f * k_samplerate_recipf;
}
