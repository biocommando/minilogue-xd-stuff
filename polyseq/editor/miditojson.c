#include "midi_reader.h"

#include <stdio.h>

#define PRINT_ONE(delta)                                                                                                            \
    if (p_note)                                                                                                                     \
    {                                                                                                                               \
        if (json_arr_count++ > 0)                                                                                                   \
            puts(",");                                                                                                              \
        printf("{\"note\": %d, \"delta\": %u, \"type\": \"%s\"}", p_note > 0 ? p_note : -p_note, delta, p_note > 0 ? "on" : "off"); \
    }

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        puts("required argument: input midifile");
        return 0;
    }
    int track_filter = -1, channel_filter = -1, sample_rate = 48000, verbose = 0;
    for (int i = 2; i < argc; i++)
    {
        if (*argv[i] == 't')
            sscanf(argv[i] + 1, "%d", &track_filter);
        if (*argv[i] == 'c')
            sscanf(argv[i] + 1, "%d", &channel_filter);
        if (*argv[i] == 's')
            sscanf(argv[i] + 1, "%d", &sample_rate);
        if (*argv[i] == 'v')
            sscanf(argv[i] + 1, "%d", &verbose);
    }
    if (verbose)
        printf("Reading file %s using params track_filter = %d, channel_filter = %d, sample_rate = %d\n", argv[1], track_filter, channel_filter, sample_rate);
    MidiFile mf;
    init_midi_file(&mf);
    mf.sample_rate = sample_rate;
    read_midi_file(argv[1], &mf);

    if (verbose)
    {
        printf("Midi info\nTrack count: %d\nTempo: %f\n"
               "Signature: %f\nticks per 1/4: %d\nsamples/tick: %d\n",
               mf.track_count, mf.tempo, mf.signature, mf.ticks_per_quarter_note, mf.samples_per_tick);
        for (int track = 0; track < mf.track_count; track++)
        {
            printf("Track %d: number of events: %u\n\n", track, mf.tracks[track].count);
        }
    }

    int json_arr_count = 0;
    int p_note = 0;
    unsigned time_total = 0;
    puts("[");
    for (int track = 0; track < mf.track_count; track++)
    {
        if (track_filter != -1 && track_filter != track)
            continue;
        for (int evt = 0; evt < mf.tracks[track].count; evt++)
        {
            MidiEvent *e = &mf.tracks[track].events[evt];
            if (channel_filter != -1 && e->data[0] & 0xf != channel_filter)
                continue;
            if ((e->data[0] & 0xf0) == 144) // note on
            {
                PRINT_ONE(e->time_delta * mf.samples_per_tick)
                p_note = e->data[1];
                time_total += e->time_delta;
            }
            if ((e->data[0] & 0xf0) == 128) // note off
            {
                PRINT_ONE(e->time_delta * mf.samples_per_tick)
                p_note = -e->data[1];
                time_total += e->time_delta;
            }
        }
    }
    unsigned full_bar_ticks = mf.ticks_per_quarter_note * 4;
    unsigned full_bars = time_total / full_bar_ticks;
    unsigned quantized_delta_to_start = full_bar_ticks - (time_total - full_bars * full_bar_ticks); // quantizes to full bars
    PRINT_ONE(quantized_delta_to_start * mf.samples_per_tick)

    puts("]");
    free_midi_file(&mf);
    return 0;
}