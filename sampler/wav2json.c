#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "../common/wav_handler/wav_handler.h"
#include "segments.h"

double note_to_frequency(const char *note)
{
    if (strlen(note) < 2)
        return 0;
    char note_name = note[0], sharp = '-';
    int octave = 4;
    const char *oct = note + 1;
    if (note[1] == '#' || note[1] == 'b')
    {
        sharp = note[1];
        oct++;
    }
    sscanf(oct, "%d",  &octave);
    int note_idx = 0;
    if (note_name == 'C')
        note_idx = 0;
    else if (note_name == 'D')
        note_idx = 2;
    else if (note_name == 'E')
        note_idx = 4;
    else if (note_name == 'F')
        note_idx = 5;
    else if (note_name == 'G')
        note_idx = 7;
    else if (note_name == 'A')
        note_idx = 9;
    else if (note_name == 'B')
        note_idx = 11;
    if (sharp == '#')
        note_idx++;
    if (sharp == 'b')
        note_idx--;
    note_idx += octave * 12;
    return 440 * pow(2, (note_idx - 69.0) / 12.0);
}

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
    uint32_t loop_idx = DATA_LEN, bits = 8;
    const char *input = NULL;
    float freq = 0;
    for (int i = 1; i < argc; i++)
    {
        if(argv[i][0] != '-')
        {
            input = argv[i];
            continue;
        }
        const char c = argv[i][1], *arg = &argv[i][2];
        if (c == 'f')
            sscanf(arg, "%f", &freq);
        if (c == 'L')
            sscanf(arg, "%u", &loop_idx);
        if (c == 'b')
            sscanf(arg, "%u", &bits);
        if (c == 'n')
            freq = note_to_frequency(arg);
    }
    printf("%f\n", freq);
    if (!input) return puts("Arguments: <input.wav> (-f<base_freq_hz>) (-L<loop_index>) (-b<bits>) (-n<note name, e.g. C4)");
    struct wav_file wav;
    if (read_wav_file(input, &wav) != 0)
        return puts("Cannot open input file");
    
    out = fopen("out.json", "w");
    if (!out)
    {
        puts("Cannot open out.json");
        free_wav_file(&wav);
        return 1;
    }
    fputs("[", out);
    print_reset();
    
    print_seg_start(SEG_WAVE);
    float maxval = 0;
    uint32_t max_val = DATA_LEN;
    if (bits == 16)
        max_val /= 2;
    for (int pass = 0; pass < 2; pass++)
    {
        for (uint32_t i = 0; i < wav.num_frames && i < max_val; i++)
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
                if (bits == 16)
                {
                    int word = v[0] * 0x7fff;
                    if (word < 0)
                        word = 0x7fff - word;
                    print_word(word, 2);
                }
                else
                {
                    int word = v[0] * 127;
                    if (word < 0)
                        word = 127 - word;
                    print_word(word, 1);
                }
            }
        }
    }
    print_reset();

    if (wav.sample_rate != 16000)
    {
        print_seg_start(SEG_SAMPLERATE);
        print_word(wav.sample_rate, 2);
        print_reset();
    }

    if (freq != 0)
    {
        print_seg_start(SEG_BASEFREQ);
        uint32_t int_freq = freq * 1e6;
        print_word(int_freq, 4);
        print_reset();
    }

    print_seg_start(SEG_LOOPIDX);
    print_word(loop_idx, 2);
    print_reset();

    if (bits != 8)
    {
        print_seg_start(SEG_BITS);
        print_word(bits, 1);
        print_reset();
    }

    fputs("]", out);
    free_wav_file(&wav);
    fclose(out);
    return 0;
}
