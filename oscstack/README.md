## Korg logue SDK: Osc Stack User Oscillator
User oscillator that has oscillator stack of 5 oscillators for the **Korg Minilogue XD** multi-engine, built using the official C/C++ **logue-sdk**.

## Building / binary distribution
A compiled and tested binary comes with this repository. To build, use the **Minilogue SDK**. To use the same workflow as I, use the legacy building method and place this repository right under `logue-sdk/platform/minilogue-xd`.

## Architecture
Has 5 oscillators at base frequency, -2 octaves, -1 octave, +1 octave and +2 octaves.

Uses library code from my other projects.

### Parameters

#### Shape parameters
- Shape:
    * Wavetable oscillator 
- Shift + Shape:
    * Noise mix

#### User parameters
- 1: Waveform:
    * 1 = sawtooth
    * 2 = square
    * 3 = triangle
    * 4 = sine (using 256 point wavetable with no interpolation, so it's kind of rough)
    * 5 = wavetable (waveform is lowpassed noise)
- 2: Base tone attenuation
- 3: -2 oct mix
- 4: -1 oct mix
- 5: +1 oct mix
- 6: +2 oct mix

## License
Original code MIT licensed (see LICENSE.md). Korg code BSD 3-Clause Licensed, license headers retained in relevant code files.

