#include "wav_handler/wav_handler.h"
#include "args.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

int main(int argc, char **argv)
{
    GET_ARG(arg_output, "-o");
    GET_ARG(arg_input, "-i");

    GET_ARG_OPT(arg_trim_start, "-ts", 1);
    GET_ARG_OPT(arg_trim_end, "-te", 1);
    int trim_start = 0;
    if (arg_trim_start >= 0)
        trim_start = atoi(argv[arg_trim_start]);
    int trim_end = 0;
    if (arg_trim_end >= 0)
        trim_end = atoi(argv[arg_trim_end]);

    struct wav_file in_wav;
    if (read_wav_file(argv[arg_input], &in_wav) != 0)
        return -1;
    FILE *f_out = fopen(argv[arg_output], "wb");
    if (!f_out)
    {
        free_wav_file(&in_wav);
        return -1;
    }
    
    fprintf(f_out, "#include \"dataoscsrc.h\"\n// from file %s\n\n", argv[arg_input]);
    fprintf(f_out, "static const int8_t _data[%d] = {\n   ", in_wav.num_frames - trim_end);

    float abs_max = 0;
    for (int i = 0; i < in_wav.num_frames; i++)
    {
        float samples[2] = {0, 0};
        wav_get_normalized(&in_wav, i, samples);
        abs_max = fabs(samples[0]) > abs_max ? fabs(samples[0]) : abs_max;
    }
    float to_char_conv_f = 127.0 / abs_max;
    int zero = 0, non_zero_found = 0;
    unsigned length = 0;
    for (int i = trim_start; i < in_wav.num_frames - trim_end; i++)
    {
        float samples[2] = {0, 0};
        wav_get_normalized(&in_wav, (unsigned)i, samples);
        const char c = samples[0] * to_char_conv_f;
        fprintf(f_out, "%3d,", c);
        if (i % 40 == 39)
            fprintf(f_out, "\n   ");
        if (c == 0)
            zero++;
        else
        {
            if (!non_zero_found)
            {
                if (zero)
                    printf("Redundant %d zeroes at start\n", zero);
                non_zero_found = 1;
            }
            zero = 0;
        }
    }
    if (zero)
        printf("Redundant %d zeroes at end\n", zero);
    fprintf(f_out, "\n};\n");

    fprintf(f_out, "const int8_t *get_waveform(uint32_t *sz, uint32_t *sr)\n{\n    *sz = sizeof(_data);\n    *sr = %u;\n    return _data;\n}", in_wav.sample_rate);

    free_wav_file(&in_wav);
    fclose(f_out);
    return 0;
}
