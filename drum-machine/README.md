## Korg logue SDK: Drum Machine User Oscillator
A lo-fi TR707ish drum machine user oscillator for the **Korg Minilogue XD** multi-engine, built using the official C/C++ **logue-sdk**.

## Building / binary distribution
A compiled and tested binary comes with this repository. To build, use the **Minilogue SDK**. To use the same workflow as I, use the legacy building method and place this repository right under `logue-sdk/platform/minilogue-xd`.

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

Has overdrive distortion for sound shaping and mix groups for setting drum sample levels.

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
    * Use high-pass filter at 6 kHz
    * Use high-pass filter at 1 kHz (filter settings mutually exclusive)
    * Use amplitude LFO (25 Hz sawtooth)

### Parameters

#### Shape parameters
- Shape:
    * Overdrive gain
- Shift + Shape:
    * "Humanization", random pitch variations applied on note on event

#### User parameters
- 1: Gate mode (stop on note release / play to end)
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
