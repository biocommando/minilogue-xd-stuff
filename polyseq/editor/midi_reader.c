#include "midi_reader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

[[maybe_unused]] static void debug_print_event(MidiEvent *this)
{
    printf("<%s %u %u %u %u>", this->end_of_track ? "y" : "n", this->data[0], this->data[1], this->data[2], this->time_delta);
}

void init_midi_file(MidiFile *mf)
{
    memset(mf, 0, sizeof(MidiFile));
    mf->tracks = NULL;
    mf->track_count = 0;
    mf->tempo = 120;
    mf->signature = 4;
    mf->ticks_per_quarter_note = 48;
    mf->samples_per_tick = 0;
    mf->sample_rate = 44100;
    mf->file_length_samples = 0;
}

/*unsigned pos = 0;
int tick_pos = 0;
int tracks_at_end = 0;*/

static void change_endianness(void *data, int length)
{
    unsigned char temp[4];
    memcpy(temp, data, length);
    unsigned char *datap = (unsigned char *)data;
    for (int i = 0; i < length; i++)
    {
        datap[length - i - 1] = temp[i];
    }
}

static void read_chunk_hdr(char *chunk_type, unsigned *length, FILE *f)
{
    /*char hdr[4];
    fread(hdr, 1, 4, f);
    chunk_type.assign(hdr, 4);*/
    fread(chunk_type, 1, 4, f);
    fread(length, sizeof(unsigned), 1, f);
    change_endianness(length, 4);
}

static unsigned read_variable_length_quantity(int *length, FILE *f)
{
    unsigned out = 0;
    // std::string debug;
    for (int i = 0; i < 4; i++)
    {
        unsigned char u;
        fread(&u, 1, 1, f);
        // debug = debug + std::to_string(u) + ';';
        unsigned char val = 0x7F & u;
        out = (out << 7) | val;
        *length = i + 1;
        if (!(u & 0x80))
        {
            break;
        }
    }
    // std::cout << "var len: " << debug << " -> " << std::to_string(out) << "\n";
    return out;
}

static void track_push_back(Track *t, MidiEvent evt)
{
    const size_t byte_size = sizeof(MidiEvent) * (t->count + 1);
    t->events = (MidiEvent *)realloc(t->events, byte_size);
    memcpy(&t->events[t->count], &evt, sizeof(MidiEvent));
    t->count++;
}

static void read_track(unsigned total_length, FILE *f, MidiFile *midi)
{
    Track events;
    events.count = 0;
    events.events = NULL;
    unsigned read_amt = 0;
    unsigned char last_status = 0;
    size_t total_time = 0;
    while (read_amt < total_length)
    {
        int length;
        unsigned time_delta = read_variable_length_quantity(&length, f);
        read_amt += length;
        unsigned char event_data;
        unsigned char next_byte;
        fread(&next_byte, 1, 1, f);
        // Running status... status is omitted if consequent events have same status
        if ((next_byte & 0x80) == 0)
        {
            fseek(f, -1, SEEK_CUR);
            event_data = last_status;
        }
        else
        {
            event_data = next_byte;
            read_amt++;
        }
        last_status = event_data;

        if (event_data == 0xF0 || event_data == 0xF7)
        {
            unsigned data_len = read_variable_length_quantity(&length, f);
            read_amt += length + data_len;
            fseek(f, data_len, SEEK_CUR);
        }
        // Meta event
        else if (event_data == 0xFF)
        {
            unsigned char type;
            fread(&type, 1, 1, f);
            unsigned data_len = read_variable_length_quantity(&length, f);
            if (type == 0x2F)
            {
                MidiEvent e;
                memset(&e, 0, sizeof(e));
                e.end_of_track = 1;
                track_push_back(&events, e);
            }
            // "Set tempo"
            else if (type == 0x51)
            {
                unsigned char tempodata[4] = {0, 0, 0, 0};
                fread(tempodata + 1, 1, 3, f);
                change_endianness(tempodata, sizeof(tempodata));
                unsigned usecs_per_quarter_note;
                memcpy(&usecs_per_quarter_note, tempodata, 4);
                midi->tempo = 1e6f / usecs_per_quarter_note * 60.0f;
            }
            read_amt += length + data_len + 1;
            fseek(f, data_len, SEEK_CUR);
        }
        else if (event_data == 0b11110010)
        {
            // Song position pointer
            fseek(f, 3, SEEK_CUR);
            read_amt += 3;
        }
        else if (event_data == 0b11110011)
        {
            // song select
            fseek(f, 1, SEEK_CUR);
            read_amt++;
        }
        else if ((event_data & 0xF0) == 0xF0)
        {
            // single byte message; do nothing
        }
        else
        {
            unsigned evt_no_ch = event_data & 0xF0;
            if (evt_no_ch == 0b11000000 || evt_no_ch == 0b11010000)
            {
                // Pgm change / after touch
                fseek(f, 1, SEEK_CUR);
                read_amt++;
            }
            else
            {
                MidiEvent e = {0};
                e.data[0] = event_data;
                fread(e.data + 1, 1, 2, f);
                e.time_delta = time_delta;
                total_time += time_delta;
                // Ignore other than note on / off
                if ((event_data & 0xF0) == 0b10000000 || (event_data & 0xF0) == 0b10010000)
                    track_push_back(&events, e);
                read_amt += 2;
            }
        }
    }
    if (events.count > 0)
    {
        midi->tracks = (Track *)realloc(midi->tracks, sizeof(Track) * (midi->track_count + 1));
        midi->tracks[midi->track_count] = events;
        midi->track_count++;
    }
    // Conversion to samples later...
    if (midi->file_length_samples < total_time)
        midi->file_length_samples = total_time;
}

void read_midi_file(const char *file, MidiFile *midi)
{
    // clear_state();
    // read_metadata(file + "_meta.ini");
    FILE *f = fopen(file, "rb");
    if (!f) return;
    unsigned short num_tracks;
    unsigned short division;
    unsigned short format;
    while (!feof(f))
    {
        char chunk_type[5] = "1234";
        unsigned length;
        read_chunk_hdr(chunk_type, &length, f);
        if (feof(f))
            break;
        const long current_pos = ftell(f);
        if (!strcmp(chunk_type, "MThd"))
        {
            fread(&format, sizeof(unsigned short), 1, f);
            change_endianness(&format, 2);
            fread(&num_tracks, sizeof(unsigned short), 1, f);
            change_endianness(&num_tracks, 2);
            fread(&division, sizeof(unsigned short), 1, f);
            change_endianness(&division, 2);
            midi->ticks_per_quarter_note = division;
            /*std::cout << "Tempo: " << std::to_string(tempo) << "\n";
            std::cout << "Samples per tick: " << std::to_string(samples_per_tick) << "\n";*/
        }
        else if (!strcmp(chunk_type, "MTrk"))
        {
            read_track(length, f, midi);
        }
        fseek(f, current_pos + length, SEEK_SET);
    }
    fclose(f);

    float ticks_per_second = midi->tempo / 60.0f * midi->ticks_per_quarter_note;
    midi->samples_per_tick = midi->sample_rate / ticks_per_second;

    midi->file_length_samples *= midi->samples_per_tick;
}

void free_midi_file(MidiFile *t)
{
    for (int i = 0; i < t->track_count; i++)
    {
        free(t->tracks[i].events);
    }
    free(t->tracks);
    init_midi_file(t);
}
