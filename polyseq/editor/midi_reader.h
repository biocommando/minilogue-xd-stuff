#pragma once

#include <stdlib.h>

// Ported from C++ code

// A single midi event (only note on/off supported)
typedef struct
{
    // Does the track end with this event?
    int end_of_track;
    // Midi event data
    unsigned char data[3];
    // Time respective to the start of the file
    unsigned time_delta;
} MidiEvent;

// An abstraction for an array of midi events within a track
typedef struct
{
    unsigned count;
    MidiEvent *events;
} Track;

/*
 * A parsed midi file containing required information for playing it back.
 */
typedef struct
{
    // All read midi tracks
    Track *tracks;
    // tracks array length
    int track_count;
    // Tempo in bpm
    float tempo;
    // The midi file time signature
    float signature;
    // Midi ticks (tick length depends on tempo) in one quarter note
    int ticks_per_quarter_note;
    // Samples per midi tick (tick length depends on tempo)
    int samples_per_tick;
    // Sample rate in Hz
    float sample_rate;
    // Total file length in samples (=last event position in samples)
    size_t file_length_samples;
} MidiFile;

/*
 * Initializes midi file structure.
 */
void init_midi_file(MidiFile *mf);

/*
 * Reads a midi file.
 * Supports reading chunk types MThd (timing information) and MTrk (tracks).
 * Supports note on, note off and set tempo events in tracks. Does not support
 * changing the tempo during track but sets the tempo to the last read tempo.
 * Should be able to read midi files that contain also other types of events but
 * the files may not play correctly.
 *
 * Note that free_midi_file needs to be called before loading another midi file.
 */
void read_midi_file(const char *file, MidiFile *midi);

/*
 * Frees any memory reserved by read_midi_file.
 */
void free_midi_file(MidiFile *t);
