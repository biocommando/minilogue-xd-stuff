## Synth voice
A basic subtractive synth voice user oscillator for the **Korg Minilogue XD** multi-engine.

## Architecture
Models one limited synth voice. Features:
- Oscillator with selectable waveform
- Sub oscillator, fixed to -1 oct using the same waveform
- "Moog" lowpass filter with cutoff and resonance
- ADSR envelope for filter cutfoff

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
    * 5 = pulse wavetable
    * 6 = thin pulse wavetable
    * 7 = sawtooth + noise wavetable
    * 8 = noise wavetable 25% length
    * 9 = noise wavetable 50% length
    * 10 = noise wavetable 75% length
    * 11 = noise wavetable 100% length
    * 12 = noise (sub oscillator either 8 or 16 times downsampled noise)
- 6: Sub oscillator mix
	* -100 % = only sub oscillator (same waveform as main)
	*    0 % = only main oscillator
	* +100 % = only sub oscillator (square wave)
