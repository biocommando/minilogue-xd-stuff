#include <stdio.h>
#include <stdint.h>
#include "../common/wav_handler/wav_handler.h"

#define DATA_LEN 30000

int seq_num = 0;
void print_word(uint32_t word, int n_bytes)
{
    while (n_bytes--)
    {
        for (int i = 0; i < 2; i++)
        {
            printf(",%u", (seq_num << 4) | (word & 0xF));
            word >>= 4;
            seq_num = (seq_num + 1) % 4;
        }
    }
    puts("");
}

void print_seg_start(unsigned segment)
{
    printf(",%u", 0x40 | segment);
    seq_num = 0;
}

void print_reset()
{
    static int first = 1;
    // Prints sequence META=7,6 to be extra sure that reset goes through
    puts(first ? "112,96" : ",112,96");
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
    
    puts("[");
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
    for (int i = 0; i < wav.num_frames && i < DATA_LEN; i++)
    {
        float v[2];
        wav_get_normalized(&wav, i, v);
        int8_t word = v[0] * 127;
        print_word((uint8_t)word, 1);
    }
    free_wav_file(&wav);
    print_reset();
    
    puts("]");
    return 0;
}
