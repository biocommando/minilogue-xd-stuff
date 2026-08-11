#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include "wav_handler.h"

int read_until_header(FILE *f, unsigned file_total_size, const char *hdr, unsigned *data_length, struct wav_file_custom_header_data *chdr)
{
    int found = 0;
    char temp_data[5];
    temp_data[4] = 0;
    while (!found &&
           // feof doesn't seem to work here for some reason
           ftell(f) < file_total_size)
    {
        fread(temp_data, 1, 4, f);
        if (!strcmp(temp_data, hdr))
            found = 1;
        fread(data_length, sizeof(unsigned), 1, f);
        if (!found)
        {
            int custom_header_found = 0;
            if (chdr)
            {
                for (struct wav_file_custom_header_data *chdr_item = chdr; chdr_item->header_name[0]; chdr_item++)
                {
                    if (!strcmp(chdr_item->header_name, temp_data))
                    {
                        custom_header_found = 1;
                        chdr_item->num_bytes = *data_length;
                        chdr_item->data = (char *)malloc(*data_length);
                        if (!chdr_item->data)
                            return -1;
                        fread(chdr_item->data, 1, *data_length, f);
                        break;
                    }
                }
            }
            if (!custom_header_found)
                fseek(f, *data_length, SEEK_CUR);
        }
    }
    return found ? 0 : -1;
}

int read_wav_file(const char *file_name, struct wav_file *wav)
{
    return read_wav_file_chdr(file_name, wav, NULL);
}

#define read_wav_file_chdr_err \
    {                          \
        if (f)                 \
            fclose(f);         \
        return -1;             \
    }

int read_wav_file_chdr(const char *file_name, struct wav_file *wav, struct wav_file_custom_header_data *chdr)
{
    wav->data = NULL;
    char temp_data[5] = "abcd";
    FILE *f = fopen(file_name, "rb");
    if (!f)
        read_wav_file_chdr_err;
    fread(temp_data, 1, 4, f);
    if (strcmp(temp_data, "RIFF"))
        read_wav_file_chdr_err;
    unsigned f_size;
    fread(&f_size, sizeof(unsigned), 1, f);
    f_size += 8;
    fread(temp_data, 1, 4, f);
    if (strcmp(temp_data, "WAVE"))
        read_wav_file_chdr_err;
    unsigned len_fmt_data;
    if (read_until_header(f, f_size, "fmt ", &len_fmt_data, chdr))
        read_wav_file_chdr_err;
    if (len_fmt_data != 16)
        read_wav_file_chdr_err;
    unsigned short fmt_type;
    fread(&fmt_type, sizeof(unsigned short), 1, f);
    if (fmt_type != 1 && fmt_type != 3)
        read_wav_file_chdr_err;
    unsigned short num_channels;
    fread(&num_channels, sizeof(unsigned short), 1, f);
    unsigned sample_rate;
    fread(&sample_rate, sizeof(unsigned), 1, f);
    // Just discard next two fields for now
    fseek(f, 4 + 2, SEEK_CUR);
    unsigned short bit_depth;
    fread(&bit_depth, sizeof(unsigned short), 1, f);
    unsigned num_bytes = 0;
    if (read_until_header(f, f_size, "data", &num_bytes, chdr))
        read_wav_file_chdr_err;
    wav->data = (char *)malloc(num_bytes);
    if (!wav->data)
        read_wav_file_chdr_err;
    fread(wav->data, 1, num_bytes, f);
    // read headers that are after data header
    if (chdr)
    {
        unsigned x;
        read_until_header(f, f_size, "", &x, chdr);
    }
    fclose(f);
    wav->num_bytes = num_bytes;
    wav->channels = num_channels;
    wav->num_frames = num_bytes / (num_channels * bit_depth / 8); // num_bytes / (channels * byte_per_sample)
    wav->bit_depth = bit_depth;
    wav->sample_rate = sample_rate;
    wav->is_float = fmt_type == 3;
    return 0;
}

int free_wav_file(struct wav_file *wav)
{
    if (wav->data)
    {
        free(wav->data);
        wav->data = NULL;
        return 0;
    }
    return -1;
}

int write_wav_file(const char *file_name, const struct wav_file *wav)
{
    return write_wav_file_chdr(file_name, wav, NULL);
}

int write_wav_file_chdr(const char *file_name, const struct wav_file *wav, const struct wav_file_custom_header_data *chdr)
{
    if (!wav->data)
        return -1;
    FILE *f = fopen(file_name, "wb");
    if (!f)
        return -1;
    fwrite("RIFF", 1, 4, f);
    unsigned total_length = 36 + wav->num_bytes;
    fwrite(&total_length, sizeof(unsigned), 1, f);
    fwrite("WAVEfmt ", 1, 8, f);
    unsigned fmt_len = 16;
    fwrite(&fmt_len, sizeof(unsigned), 1, f);
    unsigned short fmt_type = wav->is_float ? 3 : 1;
    fwrite(&fmt_type, sizeof(unsigned short), 1, f);
    unsigned short num_channels = (unsigned short)wav->channels;
    fwrite(&num_channels, sizeof(unsigned short), 1, f);
    fwrite(&wav->sample_rate, sizeof(unsigned), 1, f);
    unsigned bytes_per_sec = wav->sample_rate * wav->bit_depth / 8 * wav->channels;
    fwrite(&bytes_per_sec, sizeof(unsigned), 1, f);
    unsigned short bytes_per_frame = wav->bit_depth / 8 * wav->channels;
    fwrite(&bytes_per_frame, sizeof(unsigned short), 1, f);
    unsigned short bit_depth = (unsigned short)wav->bit_depth;
    fwrite(&bit_depth, sizeof(unsigned short), 1, f);
    if (chdr)
    {
        for (const struct wav_file_custom_header_data *hdr = chdr; hdr->header_name[0]; hdr++)
        {
            fwrite(hdr->header_name, 1, 4, f);
            fwrite(&hdr->num_bytes, sizeof(unsigned), 1, f);
            fwrite(hdr->data, 1, hdr->num_bytes, f);
        }
    }
    fwrite("data", 1, 4, f);
    fwrite(&wav->num_bytes, sizeof(unsigned), 1, f);
    fwrite(wav->data, 1, wav->num_bytes, f);
    fclose(f);
    return 0;
}

int create_wav_file(struct wav_file *wav, unsigned num_frames, unsigned channels, unsigned bit_depth, unsigned sample_rate)
{
    unsigned num_bytes = bit_depth / 8 * num_frames * channels;
    wav->data = (char *)malloc(num_bytes);
    if (!wav->data)
        return -1;
    memset(wav->data, 0, num_bytes);
    wav->channels = channels;
    wav->num_bytes = num_bytes;
    wav->num_frames = num_frames;
    wav->bit_depth = bit_depth;
    wav->sample_rate = sample_rate;
    wav->is_float = 0;
    return 0;
}

int wav_get_normalized(const struct wav_file *wav, unsigned sample_idx, float *value)
{
    if (!wav->data)
        return -1;
    unsigned byte_depth = wav->bit_depth / 8;
    unsigned bytes_per_sample = byte_depth * wav->channels;
    unsigned data_idx = bytes_per_sample * sample_idx;
    if (data_idx > wav->num_bytes - bytes_per_sample)
        return -1;
    unsigned i;
    for (i = 0; i < wav->channels; i++)
    {
        float val = 0;
        const char *data_p = &wav->data[data_idx + byte_depth * i];
        if (byte_depth == 1)
        {
            val = (unsigned char)*data_p;
            val /= 0xFF;
            val = val * 2 - 1;
        }
        else if (byte_depth == 2)
        {
            val = *(short *)data_p;
            val /= 0x8000 - 1;
        }
        else if (byte_depth == 3)
        {
            // 24 bit value
            // 01 00 00 (1)
            // 01 00 80 (-2147483324)
            // 32 bit value
            // 01 00 00 xx
            // 01 00 80 xx
            int val32b = *(int *)data_p;
            const int is_signed = val32b & 0x00800000;
            if (is_signed)
                val32b |= 0xFF000000;
            else
                val32b &= 0x00FFFFFF;
            val = val32b;
            val /= 0x800000 - 1;
        }
        else if (byte_depth == 4)
        {
            if (wav->is_float)
                val = *(float *)data_p;
            else
            {
                val = *(int *)data_p;
                val /= 0x80000000 - 1;
            }
        }
        else if (byte_depth == 8 && wav->is_float)
        {
            val = *(double *)data_p;
        }
        value[i] = val;
    }
    return 0;
}

int wav_set_normalized(struct wav_file *wav, unsigned sample_idx, float *value)
{
    if (!wav->data)
        return -1;
    unsigned byte_depth = wav->bit_depth / 8;
    unsigned bytes_per_sample = byte_depth * wav->channels;
    unsigned data_idx = bytes_per_sample * sample_idx;
    if (data_idx > wav->num_bytes - bytes_per_sample)
        return -1;
    unsigned i;
    for (i = 0; i < wav->channels; i++)
    {
        double val = value[i];
        val = val > 1 ? 1 : (val < -1 ? -1 : val);
        char *data_p = &wav->data[data_idx + byte_depth * i];
        if (byte_depth == 1)
        {
            val = (val + 1) / 2;
            val *= 0xFF;
            *(unsigned char *)data_p = val;
        }
        else if (byte_depth == 2)
        {
            val *= 0x8000 - 1;
            *(short *)data_p = val;
        }
        else if (byte_depth == 3)
        {
            val *= 0x800000 - 1;
            int val32b = val;
            data_p[0] = 0xFF & val32b;
            data_p[1] = 0xFF & (val32b >> 8);
            data_p[2] = 0xFF & (val32b >> 16);
        }
        else if (byte_depth == 4)
        {
            if (wav->is_float)
                *(float *)data_p = val;
            else
            {
                val *= 0x80000000 - 1;
                *(int *)data_p = val;
            }
        }
        else if (byte_depth == 8 && wav->is_float)
        {
            *(double *)data_p = val;
        }
    }
    return 0;
}
