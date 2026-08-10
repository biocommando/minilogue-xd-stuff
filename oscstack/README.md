## Osc Stack
User oscillator that has 3 oscillators in the following configuration:
- 1 oscillator at base frequency
- 2 oscillators at selected octave offset that can be detuned up to 0.67 semitones apart

### Parameters

#### Shape parameters
- Shape:
    * Wavetable oscillator wavetable position
- Shift + Shape:
    * Noise mix

#### User parameters
- 1: Osc1 Waveform:
    * 1 = sawtooth
    * 2 = square
    * 3 = triangle
    * 4 = sine (using 256 point wavetable with no interpolation, so it's kind of rough)
    * 5 = wavetable (waveform is lowpassed noise)
- 2: Osc1 tone attenuation
- 3: Osc2 mix
- 4: Osc2 Waveform (same selections as for Osc1)
- 5: Osc2 Octave (-2 to +2)
- 6: Osc2 Detune
