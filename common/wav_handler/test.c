/*
    The idea of this program is to output wavefiles in multiple wav formats.
    One can play the generated files back using a 3rd party program to verify
    the functionality of the library.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "wav_handler.h"

#define print_field(f) printf(#f " = %u\n", (unsigned)wav->f)

void print_wav(struct wav_file *wav)
{
    if (!wav->data)
    {
        printf("Wave data not initialized\n");
        return;
    }
    printf("--- wave file ---\n");
    print_field(num_bytes);
    print_field(channels);
    print_field(num_frames); // num_bytes / (channels * byte_per_sample)
    print_field(sample_rate);
    print_field(bit_depth);
    print_field(is_float);
    printf("-----------------\n");
}

#define PRINT_ERR(expression) printf(#expression ": %s\n", (expression) ? "ERROR" : "SUCCESS")
#define PRINT_ON_ERR(expression) \
    if (expression)              \
    printf(#expression ": ERROR\n")

int main(int argc, char **argv)
{
    struct wav_file wav;

    printf("Test 8 bit int 22050 Hz mono audio\n");
    PRINT_ON_ERR(create_wav_file(&wav, 2 * 22050, 1, 8, 22050));
    for (int i = 0; i < 2 * 22050; i++)
    {
        float val = sin(i / 15.0);
        PRINT_ON_ERR(wav_set_normalized(&wav, i, &val));
    }
    PRINT_ON_ERR(write_wav_file("8bit_22050Hz_int_mono.wav", &wav));
    PRINT_ON_ERR(free_wav_file(&wav));

    printf("Test 16 bit int 44100 Hz mono audio\n");
    PRINT_ON_ERR(create_wav_file(&wav, 44100 * 2, 1, 16, 44100));

    for (int i = 0; i < 44100 * 2; i++)
    {
        float val = sin(i / 30.0);
        PRINT_ON_ERR(wav_set_normalized(&wav, i, &val));
    }
    PRINT_ON_ERR(write_wav_file("16bit_44100Hz_int_mono.wav", &wav));
    PRINT_ON_ERR(free_wav_file(&wav));

    printf("Test 24 bit int 44100 Hz mono audio\n");
    PRINT_ON_ERR(create_wav_file(&wav, 44100 * 2, 1, 24, 44100));

    for (int i = 0; i < 44100 * 2; i++)
    {
        float val = sin(i / 30.0);
        PRINT_ON_ERR(wav_set_normalized(&wav, i, &val));
    }
    PRINT_ON_ERR(write_wav_file("24bit_44100Hz_int_mono.wav", &wav));
    PRINT_ON_ERR(free_wav_file(&wav));

    printf("Test 32 bit int 44100 Hz mono audio\n");
    PRINT_ON_ERR(create_wav_file(&wav, 44100 * 2, 1, 32, 44100));

    for (int i = 0; i < 44100 * 2; i++)
    {
        float val = sin(i / 30.0);
        PRINT_ON_ERR(wav_set_normalized(&wav, i, &val));
    }
    PRINT_ON_ERR(write_wav_file("32bit_44100Hz_int_mono.wav", &wav));
    PRINT_ON_ERR(free_wav_file(&wav));

    printf("Test 32 bit float 44100 Hz mono audio\n");
    PRINT_ON_ERR(create_wav_file(&wav, 44100 * 2, 1, 32, 44100));
    wav.is_float = 1;

    for (int i = 0; i < 44100 * 2; i++)
    {
        float val = sin(i / 30.0);
        PRINT_ON_ERR(wav_set_normalized(&wav, i, &val));
    }
    PRINT_ON_ERR(write_wav_file("32bit_44100Hz_float_mono.wav", &wav));
    PRINT_ON_ERR(free_wav_file(&wav));

    printf("Test 64 bit float 44100 Hz mono audio\n");
    PRINT_ON_ERR(create_wav_file(&wav, 44100 * 2, 1, 64, 44100));
    wav.is_float = 1;

    for (int i = 0; i < 44100 * 2; i++)
    {
        float val = sin(i / 30.0);
        PRINT_ON_ERR(wav_set_normalized(&wav, i, &val));
    }
    PRINT_ON_ERR(write_wav_file("64bit_44100Hz_float_mono.wav", &wav));
    PRINT_ON_ERR(free_wav_file(&wav));

    // Stereo audio

    printf("Test 16 bit int 44100 Hz stereo audio\n");
    PRINT_ON_ERR(create_wav_file(&wav, 44100 * 2, 2, 16, 44100));

    for (int i = 0; i < 44100 * 2; i++)
    {
        float val = sin(i / 30.0);
        float chs[] = {val, -val};
        PRINT_ON_ERR(wav_set_normalized(&wav, i, chs));
    }
    PRINT_ON_ERR(write_wav_file("16bit_44100Hz_int_stereo.wav", &wav));
    PRINT_ON_ERR(free_wav_file(&wav));

    // Read test: read all data and write it in new format

    struct wav_file wav2;

    printf("Read test 8 bit int mono audio\n");
    PRINT_ON_ERR(read_wav_file("8bit_22050Hz_int_mono.wav", &wav));
    PRINT_ON_ERR(create_wav_file(&wav2, wav.num_frames, wav.channels, 16, wav.sample_rate));
    for (int i = 0; i < wav.num_frames; i++)
    {
        float val = 0;
        PRINT_ON_ERR(wav_get_normalized(&wav, i, &val));
        PRINT_ON_ERR(wav_set_normalized(&wav2, i, &val));
    }
    PRINT_ON_ERR(write_wav_file("read_8bit_mono.wav", &wav2));
    PRINT_ON_ERR(free_wav_file(&wav));
    PRINT_ON_ERR(free_wav_file(&wav2));

    printf("Read test 16 bit int mono audio\n");
    PRINT_ON_ERR(read_wav_file("16bit_44100Hz_int_mono.wav", &wav));
    PRINT_ON_ERR(create_wav_file(&wav2, wav.num_frames, wav.channels, 8, wav.sample_rate));
    for (int i = 0; i < wav.num_frames; i++)
    {
        float val = 0;
        PRINT_ON_ERR(wav_get_normalized(&wav, i, &val));
        PRINT_ON_ERR(wav_set_normalized(&wav2, i, &val));
    }
    PRINT_ON_ERR(write_wav_file("read_16bit_mono.wav", &wav2));
    PRINT_ON_ERR(free_wav_file(&wav));
    PRINT_ON_ERR(free_wav_file(&wav2));

    printf("Read test 24 bit int mono audio\n");
    PRINT_ON_ERR(read_wav_file("24bit_44100Hz_int_mono.wav", &wav));
    PRINT_ON_ERR(create_wav_file(&wav2, wav.num_frames, wav.channels, 16, wav.sample_rate));
    for (int i = 0; i < wav.num_frames; i++)
    {
        float val = 0;
        PRINT_ON_ERR(wav_get_normalized(&wav, i, &val));
        PRINT_ON_ERR(wav_set_normalized(&wav2, i, &val));
    }
    PRINT_ON_ERR(write_wav_file("read_24bit_mono.wav", &wav2));
    PRINT_ON_ERR(free_wav_file(&wav));
    PRINT_ON_ERR(free_wav_file(&wav2));

    printf("Read test 32 bit int mono audio\n");
    PRINT_ON_ERR(read_wav_file("32bit_44100Hz_int_mono.wav", &wav));
    PRINT_ON_ERR(create_wav_file(&wav2, wav.num_frames, wav.channels, 16, wav.sample_rate));
    for (int i = 0; i < wav.num_frames; i++)
    {
        float val = 0;
        PRINT_ON_ERR(wav_get_normalized(&wav, i, &val));
        PRINT_ON_ERR(wav_set_normalized(&wav2, i, &val));
    }
    PRINT_ON_ERR(write_wav_file("read_32bit_mono.wav", &wav2));
    PRINT_ON_ERR(free_wav_file(&wav));
    PRINT_ON_ERR(free_wav_file(&wav2));

    printf("Read test 32 bit float mono audio\n");
    PRINT_ON_ERR(read_wav_file("32bit_44100Hz_float_mono.wav", &wav));
    PRINT_ON_ERR(create_wav_file(&wav2, wav.num_frames, wav.channels, 16, wav.sample_rate));
    for (int i = 0; i < wav.num_frames; i++)
    {
        float val = 0;
        PRINT_ON_ERR(wav_get_normalized(&wav, i, &val));
        PRINT_ON_ERR(wav_set_normalized(&wav2, i, &val));
    }
    PRINT_ON_ERR(write_wav_file("read_32bit_float_mono.wav", &wav2));
    PRINT_ON_ERR(free_wav_file(&wav));
    PRINT_ON_ERR(free_wav_file(&wav2));

    printf("Read test 64 bit float mono audio\n");
    PRINT_ON_ERR(read_wav_file("64bit_44100Hz_float_mono.wav", &wav));
    PRINT_ON_ERR(create_wav_file(&wav2, wav.num_frames, wav.channels, 16, wav.sample_rate));
    for (int i = 0; i < wav.num_frames; i++)
    {
        float val = 0;
        PRINT_ON_ERR(wav_get_normalized(&wav, i, &val));
        PRINT_ON_ERR(wav_set_normalized(&wav2, i, &val));
    }
    PRINT_ON_ERR(write_wav_file("read_64bit_float_mono.wav", &wav2));
    PRINT_ON_ERR(free_wav_file(&wav));
    PRINT_ON_ERR(free_wav_file(&wav2));

    printf("Read test 16 bit int stereo audio\n");
    PRINT_ON_ERR(read_wav_file("16bit_44100Hz_int_stereo.wav", &wav));
    PRINT_ON_ERR(create_wav_file(&wav2, wav.num_frames, wav.channels, 8, wav.sample_rate));
    for (int i = 0; i < wav.num_frames; i++)
    {
        float val[] = {0, 0};
        PRINT_ON_ERR(wav_get_normalized(&wav, i, val));
        PRINT_ON_ERR(wav_set_normalized(&wav2, i, val));
    }
    PRINT_ON_ERR(write_wav_file("read_16bit_stereo.wav", &wav2));
    PRINT_ON_ERR(free_wav_file(&wav));
    PRINT_ON_ERR(free_wav_file(&wav2));

    printf("Read test: wave data with additional header etc. data\n");
    struct wav_file_custom_header_data custom_headers[4] = {
        {"fact", 0, NULL},
        {"PEAK", 0, NULL},
        {"LIST", 0, NULL},
        {"", 0, NULL},
    };
    PRINT_ON_ERR(read_wav_file_chdr("sine.dat", &wav, custom_headers));

    printf("Custom header data:\n");
    for (int i = 0; custom_headers[i].header_name[0]; i++)
    {
        printf("Header %s, size = %u, data: ", custom_headers[i].header_name, custom_headers[i].num_bytes);
        for (int j = 0; j < custom_headers[i].num_bytes; j++)
            printf("%2x ", ((unsigned)custom_headers[i].data[j]) & 0xFF);
        printf("\n");
    }

    PRINT_ON_ERR(create_wav_file(&wav2, wav.num_frames, wav.channels, 16, wav.sample_rate));
    for (int i = 0; i < wav.num_frames; i++)
    {
        float val = 0;
        PRINT_ON_ERR(wav_get_normalized(&wav, i, &val));
        PRINT_ON_ERR(wav_set_normalized(&wav2, i, &val));
    }
    PRINT_ON_ERR(write_wav_file_chdr("read_sine.wav", &wav2, custom_headers));
    PRINT_ON_ERR(free_wav_file(&wav));
    PRINT_ON_ERR(free_wav_file(&wav2));

    free(custom_headers[0].data);
    free(custom_headers[1].data);
    free(custom_headers[2].data);

    return 0;
}
