#include "wav_handler/wav_handler.h"
#include "args.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define def_compress int compress(const char *out, const char *in, int trim_start, int trim_end)
#define def_decompress int decompress(const char *out, const char *in)
#define def_decompress_to_c int decompress_to_c(const char *out, const char *in)

def_compress;
def_decompress;
def_decompress_to_c;

#define FAIL(msg)                   \
    {                               \
        printf("ERROR: " msg "\n"); \
        return 1;                   \
    }

#define TRY(ex)  \
    if (ex != 0) \
    FAIL(#ex)

int main(int argc, char **argv)
{
    GET_ARG(arg_mode, "-m");
    GET_ARG_OPT(arg_trim_start, "-ts", 1);
    GET_ARG_OPT(arg_trim_end, "-te", 1);
    int trim_start = 0;
    if (arg_trim_start >= 0)
        trim_start = atoi(argv[arg_trim_start]);
    int trim_end = 0;
    if (arg_trim_end >= 0)
        trim_end = atoi(argv[arg_trim_end]);
    if (argv[arg_mode][0] == 'c')       // compress
    {
        GET_ARG(arg_output, "-o");
        GET_ARG(arg_input, "-i");
    TRY(compress(argv[arg_output], argv[arg_input], trim_start, trim_end))}
    if (argv[arg_mode][0] == 'd')       // decompress
    {
        GET_ARG(arg_output, "-o");
        GET_ARG(arg_input, "-i");
    TRY(decompress(argv[arg_output], argv[arg_input]))}
    if (argv[arg_mode][0] == 'r')       // roundtrip
    {
        GET_ARG(arg_output, "-o");
        GET_ARG(arg_input, "-i");

        char *bin_out = malloc(strlen(argv[arg_input]) + 1 + 4);
        sprintf(bin_out, "%s.bin", argv[arg_input]);
    TRY(compress(bin_out, argv[arg_input], trim_start, trim_end)) TRY(decompress(argv[arg_output], bin_out))}
    if (argv[arg_mode][0] == 'C')       // code
    {
        GET_ARG(arg_output, "-o");
        GET_ARG(arg_input, "-i");

    TRY(decompress_to_c(argv[arg_output], argv[arg_input]))}
    return 0;
}

def_compress
{
    struct wav_file in_wav;
    if (read_wav_file(in, &in_wav) != 0)
        return -1;
    FILE *f_out = fopen(out, "wb");
    if (!f_out)
    {
        free_wav_file(&in_wav);
        return -1;
    }
    int trimmed_length = in_wav.num_frames - trim_end - trim_start;
    int total_length = trimmed_length + trimmed_length % 5;
    fwrite(&total_length, sizeof(total_length), 1, f_out);
    fwrite(&in_wav.sample_rate, sizeof(unsigned), 1, f_out);
    double inc = trimmed_length / (double) total_length;
    int _i = 0, num_positive = 0;
    char _wbuf[5];
    float abs_max = 0;
    for (int i = 0; i < in_wav.num_frames; i++)
    {
        float samples[2] = { 0, 0 };
        wav_get_normalized(&in_wav, i, samples);
        abs_max = fabs(samples[0]) > abs_max ? fabs(samples[0]) : abs_max;
    }
    float to_char_conv_f = 127.0 / abs_max;
    int zero = 0, non_zero_found = 0;
    for (double i = trim_start; i < in_wav.num_frames - trim_end; i += inc)
    {
        float samples[2] = { 0, 0 };
        wav_get_normalized(&in_wav, (unsigned) i, samples);
        _wbuf[_i % 5] = samples[0] * to_char_conv_f;
        if (*samples > 0)
            num_positive++;
        if (_i % 5 == 4)
        {
            unsigned short _word = 0;
            int sign = 0;
            if (num_positive < 3)
                sign = 1;
            for (int wi = 0; wi < 5; wi++)
            {
                unsigned char b = abs(_wbuf[wi]);
                if (b > 127)
                    b = 127;
                _word |= ((b >> 4) & 0x7) << (wi * 3);
            }
            if (_word == 0)
                zero++;
            else
            {
                if (!non_zero_found)
                {
                    if (zero)
                        printf("Redundant %d zeroes at start\n", zero * 5);
                    non_zero_found = 1;
                }
                zero = 0;
            }
            _word |= sign << 15;
            fwrite(&_word, sizeof(_word), 1, f_out);
            num_positive = sign;
        }
        _i++;
    }
    if (zero)
        printf("Redundant %d zeroes at end\n", zero * 5);
    fclose(f_out);
    free_wav_file(&in_wav);
    return 0;
}

def_decompress
{
    FILE *f_in = fopen(in, "rb");
    if (!f_in)
    {
        return -1;
    }
    int total_length;
    unsigned sr;
    fread(&total_length, sizeof(total_length), 1, f_in);
    fread(&sr, sizeof(sr), 1, f_in);
    struct wav_file out_wav;
    create_wav_file(&out_wav, total_length, 1, 16, sr);
    int _i = 0;
    for (int i = 0; i < total_length / 5; i++)
    {
        unsigned short _word = 0;
        fread(&_word, sizeof(_word), 1, f_in);
        int sign = _word & 0x8000;
        for (int wi = 0; wi < 5; wi++)
        {
            float sample = ((int) ((_word >> (wi * 3)) & 0x7)) / 7.0;
            if (sign)
                sample = -sample;
            wav_set_normalized(&out_wav, _i, &sample);
            _i++;
        }
    }
    int res = write_wav_file(out, &out_wav);

    free_wav_file(&out_wav);
    fclose(f_in);

    return res;
}

def_decompress_to_c
{
    FILE *f_in = fopen(in, "rb");
    if (!f_in)
    {
        return -1;
    }
    FILE *f_out = fopen(out, "w");
    if (!f_out)
    {
        fclose(f_in);
        return -1;
    }

    int total_length;
    unsigned sr;
    fread(&total_length, sizeof(total_length), 1, f_in);
    fread(&sr, sizeof(sr), 1, f_in);

    int num_words = total_length / 5;
    fprintf(f_out, "static const unsigned _length = %d;\n", num_words);
    fprintf(f_out, "static const unsigned short _data[%d] = {\n   ", num_words);

    for (int i = 0; i < num_words; i++)
    {
        unsigned short _word = 0;
        fread(&_word, sizeof(_word), 1, f_in);
        fprintf(f_out, "%6u,", _word);
        if (i % 12 == 11)
            fprintf(f_out, "\n   ");
    }
    fprintf(f_out, "\n};\n");

    fclose(f_in);
    fclose(f_out);
    return 0;
}
