## Korg logue SDK: Drum Machine User Oscillator
A drum machine user oscillator that has lo-fi samples and synthesized drums.

## Architecture
Has 2 drum engines that can be mixed with the shape parameters. One uses lo-fi PCM samples and the other uses a simple synth engine.

The 12 different drum sounds are:
- kick (2 variations)
- snare (2 variations)
- handclap
- closed hihat
- open hihat
- crash
- tambourine
- cowbell
- rimshot
- tom

The drum sounds can be played back with different playback speeds (pitch) using different octaves. In each octave the note to sample mapping is the following:
- C = kick 1
- C# = kick 2
- D = snare 1
- D# = snare 2
- E = handclap
- F = closed hihat
- F# = tambourine
- G = open hihat
- G# = cowbell
- A = crash
- A# = rimshot
- B = tom

Has overdrive distortion for sound shaping and mix groups for setting drum voice levels.

### Sample playback engine
Uses a custom raw waveform format that compresses 5 4-bit samples into one 16 bit word.
One word consists of data:
```
offset | data
-------------------------------------
0      | sign bit for the whole word
1      | sample 1 (3 bits)
4      | sample 2 (3 bits)
7      | sample 3 (3 bits)
10     | sample 4 (3 bits)
13     | sample 5 (3 bits)
```

The sample playback speed varies from 0.5 to ~1.74 depending on the played octave.

### Drum synth engine
The engine consists of a single oscillator with triangle, sawtooth and square waveforms and a noise source. Each drum sound is generated using the following parameters:
- Oscillator waveform
- Oscillator pitch
    * Affected by the played octave
- Oscillator pitch modulation envelope length
- Oscillator mix
- Noise mix
- Master volume envelope length
- Flags:
    * Use high-pass filter (2 bits) (off, 1 kHz, 3 kHz, 6 kHz)
    * Amplitude LFO (2 bits) (Off, 12, 25 or 65 Hz sawtooth)
    * Make envelopes longer

### Parameters

#### Shape parameters
- Shape:
    * Sample playback / synth drums mix. 0% = only samples, 100% = only synth drums.
- Shift + Shape:
    * Overdrive gain

#### User parameters
- 1: Synth drum engine variation
	- Flips bits in drum synth engine's "flags" parameter to create
	  slight variations in how the whole kit sounds like.
- 2: Kick attenuation
- 3: Snare & handclap attenuation
- 4: Hihats attenuation
- 5: Crash attenuation
- 6: Percussions attenuation (tambourine, cowbell, rimshot, tom)

## Sample layout reference

| Sample     | Key | Mixing group |
|------------|-----|--------------|
| kick1      | C   | kick         |
| kick2      | C#  | kick         |
| snare1     | D   | snare        |
| snare2     | D#  | snare        |
| handclap   | E   | snare        |
| cl hihat   | F   | hihat        |
| op hihat   | G   | hihat        |
| crash      | A   | crash        |
| tambourine | F#  | perc         |
| cowbell    | G#  | perc         |
| rimshot    | A#  | perc         |
| tom        | B   | perc         |
