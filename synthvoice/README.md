## Korg logue SDK: Synth voice User Oscillator
A basic subtractive synth voice user oscillator for the **Korg Minilogue XD** multi-engine, built using the official C/C++ **logue-sdk**.

## Building / binary distribution
A compiled and tested binary comes with this repository. To build, use the **Minilogue SDK**. To use the same workflow as I, use the legacy building method and place this repository right under `logue-sdk/platform/minilogue-xd`.

## Architecture
Models one limited synth voice. Features:
- Oscillator with selectable waveform
- Sub oscillator, fixed to -1 oct using the same waveform
- "Moog" lowpass filter with cutoff and resonance
- ADSR envelope for filter cutfoff

Uses library code from my other projects.

### Parameters

#### Shape parameters
- Shape:
    * Filter cutoff
- Shift + Shape:
    * Filter resonance

#### User parameters
- 1: Filter Envelope Attack length (0-4 sec)
- 2: Filter Envelope Decay length (0-4 sec)
- 3: Filter Envelope Sustain level
- 4: Filter Envelope Release length (0-4 sec)
- 5: Waveform:
    * 1 = sawtooth
    * 2 = square
    * 3 = triangle
    * 4 = sine (using 256 point wavetable with no interpolation, so it's kind of rough)
- 6: Sub oscillator mix

## License
Original code MIT licensed (see LICENSE.md). Korg code BSD 3-Clause Licensed, license headers retained in relevant code files.

