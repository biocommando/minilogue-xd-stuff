## Korg logue SDK: Drum Machine User Oscillator
A lo-fi TR707ish drum machine user oscillator for the **Korg Minilogue XD** multi-engine, built using the official C/C++ **logue-sdk**.

## Building / binary distribution
A compiled and tested binary comes with this repository. To build, use the **Minilogue SDK**. To use the same workflow as I, use the legacy building method and place this repository right under `logue-sdk/platform/minilogue-xd`.

## Architecture
Plays back 12 different lo-fi drum samples:
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

Uses a custom raw waveform format that compresses 5 4-bit samples into one 16 bit word. Has overdrive distortion and an optional volume envelope for soundshaping. Has mix groups for setting drum sample levels.

The samples can be played back with different playback speeds (pitch) using different octaves. In each octave the note to sample mapping is the following:
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

### Parameters

#### Shape parameters
- Shape:
    * Overdrive gain
- Shift + Shape:
    * Decay length

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


## License
Original code MIT licensed (see LICENSE.md). Korg code BSD 3-Clause Licensed, license headers retained in relevant code files.

