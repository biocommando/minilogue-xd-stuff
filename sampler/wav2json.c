#include <stdio.h>
#include <stdint.h>
#include "../common/wav_handler/wav_handler.h"

#define DATA_LEN 30000

FILE *out;
int seq_num = 0;
void print_word(uint32_t word, int n_bytes)
{
    int shift = (n_bytes * 2 - 1) * 4;
    while (n_bytes--)
    {
        for (int i = 0; i < 2; i++)
        {
            fprintf(out, ",%u", (seq_num << 4) | ((word >> shift) & 0xF));
            seq_num = (seq_num + 1) % 4;
            shift -= 4;
        }
    }
    fputs("", out);
}

void print_seg_start(unsigned segment)
{
    fprintf(out, ",%u", 0x40 | segment);
    seq_num = 0;
}

void print_reset()
{
    static int first = 1;
    // Prints sequence META=7,6 to be extra sure that reset goes through
    fputs(first ? "112,96" : ",112,96", out);
    first = 0;
}

int main(int argc, char **argv)
{
    if (argc < 3) return puts("Arguments required: input.wav base_freq_hz optional:loop_index");
    uint32_t loop_idx = DATA_LEN;
    if (argc > 3)
        sscanf(argv[3], "%u", &loop_idx);
    struct wav_file wav;
    if (read_wav_file(argv[1], &wav) != 0)
        return puts("Cannot open input file");
    float freq = 440;
    sscanf(argv[2], "%f", &freq);
    
    out = fopen("out.json", "w");
    if (!out)
    {
        puts("Cannot open out.json");
        free_wav_file(&wav);
        return 1;
    }
    fputs("[", out);
    print_reset();
    print_seg_start(1);
    print_word(wav.sample_rate, 2);
    print_reset();

    print_seg_start(2);
    uint32_t int_freq = freq * 1e6;
    print_word(int_freq, 4);
    print_reset();

    print_seg_start(3);
    print_word(loop_idx, 2);
    print_reset();
    
    print_seg_start(4);
    float maxval = 0;
    for (int pass = 0; pass < 2; pass++)
    {
        for (int i = 0; i < wav.num_frames && i < DATA_LEN; i++)
        {
            float v[2];
            wav_get_normalized(&wav, i, v);
            if (pass == 0)
            {
                if (maxval < v[0])
                    maxval = v[0];
                if (maxval < -v[0])
                    maxval = -v[0];
            }
            else
            {
                v[0] /= maxval;
                int word = v[0] * 127;
                if (word < 0)
                    word = 127 - word;
                print_word(word, 1);
            }
        }
    }
    free_wav_file(&wav);
    print_reset();
    
    fputs("]", out);
    fclose(out);
    return 0;
}
