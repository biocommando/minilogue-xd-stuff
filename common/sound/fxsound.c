#include "../wav_handler/wav_handler.h"

#include "modfx_api.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#define LIMQ31(x) \
  x = x >= 0.99999997f ? 0.99999997f : x

int main(int argc, char **argv)
{  
  if (argc < 5)
  {
    puts("Required arguments:");
    puts("input wav");
    puts("output wav");
    puts("time param");
    puts("depth param");
    return 1;
  }
#ifdef MODFX_DRUMS_DEBUG
    extern void set_pattern(const uint8_t *p);
    static uint8_t pattern[17];
    FILE *f = fopen("pattern.txt", "r");
    if (!f)
    {
        puts("pattern.txt required if built with MODFX_DRUMS_DEBUG option!");
        return 1;
    }
    for (int i = 0; i < sizeof(pattern);)
    {
        char buf[128];
        unsigned u = 0;
        fscanf(f, "%s", buf);
        if (sscanf(buf, "%u", &u) == 1)
        {
            pattern[i] = u;
            i++;
        }
        if (feof(f))
        {
            puts("failed to read pattern.txt");
            fclose(f);
            return 1;
        }
    }
    puts("Read pattern:");
    for (int i = 0; i < sizeof(pattern); i++)
        printf("%u, ", pattern[i]);
    puts("");
    fclose(f);
#endif
  
  struct wav_file w_in, w_out;
  if (read_wav_file(argv[1], &w_in) != 0)
  {
    puts("Can't read input wav file");
    return 1;
  }
  const double w_in_sr_ratio = (double)w_in.sample_rate / (double)k_samplerate;
  const unsigned out_num_frames = w_in.num_frames / w_in_sr_ratio;
  create_wav_file(&w_out, out_num_frames, 2, 16, k_samplerate);
  
  MODFX_INIT(0, 0);
  
  float p_time = 0, p_depth = 0;
  sscanf(argv[3], "%f", &p_time);
  sscanf(argv[4], "%f", &p_depth);
  LIMQ31(p_time);
  LIMQ31(p_depth);
  MODFX_PARAM(k_user_modfx_param_time, f32_to_q31(p_time));
  MODFX_PARAM(k_user_modfx_param_depth, f32_to_q31(p_depth));

#ifdef MODFX_DRUMS_DEBUG
    set_pattern(pattern);
#endif


  int sample_i = 0;
  int bsize = 8;
  float main_x[128], main_y[128];
  float sub_x[128], sub_y[128];
  memset(sub_x, 0, sizeof(sub_x));
  memset(sub_y, 0, sizeof(sub_y));
  while (sample_i < w_out.num_frames)
  {
      for (int i = 0; i < bsize; i++)
      {
        float f[2];
        wav_get_normalized_linint(&w_in, (sample_i + i) * w_in_sr_ratio, f);
        main_x[i * 2] = f[0];
        main_x[i * 2 + 1] = f[1];
      }
      MODFX_PROCESS(main_x, main_y, sub_x, sub_y, bsize);
      for (int i = 0; i < bsize; i++)
      {
          wav_set_normalized(&w_out, sample_i, &main_y[i * 2]);
          sample_i++;
      }
      bsize *= 2;
      if (bsize > 64)
          bsize = 8;
  }
  
  if (write_wav_file(argv[2], &w_out) != 0)
      puts("Writing output file failed");
  free_wav_file(&w_in);
  free_wav_file(&w_out);
  return 0;
}
