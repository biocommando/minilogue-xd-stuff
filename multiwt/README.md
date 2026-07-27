## Wavetable synthesizer
A wavetable synthesizer focused on switching between waveforms.

## Architecture
Has 40 128-point wavetables micro-sampled from random songs. Shape parameter selects the wavetable that is crossfaded into the new
selection during the next osc cycle. It has 20 predefined wavetable playback order tables so you can control the modulated spectral character
using it. It comes with an extra LFO that can be used for additional shape modulation.

### Parameters

#### Shape parameters
- Shape:
    * Wavetable selection
- Shift + Shape:
    * Wavetable window (0 % = full wavetable, 100 % = 5 % of the length (~6 samples))

#### User parameters
- 1: LFO intensity
- 2: LFO frequency (0.01...1.01 Hz)
- 3: LFO target, bitmask (1..7):
	- 001 = Wavetable window
	- 010 = Wavetable position
	- 100 = Wavetable selection
- 4: Stickiness
    * Affects how long the engine holds one wavetable before switching.
    * Range: 0 % = immediate, 100 % = 1 second delay
- 5: Wavetable playback position
    * Has a bigger impact with smaller wavetable windows. The looped waveform is in range:
      position..position+window. Window is a ratio of total wavetable length and the position
      controls the playback starting position in the remainder of the wavetable. (The start position
      will not wrap, so e.g. with window=0.2, the position will be in range 0..0.8).
- 6: Wavetable ordering

## License
Original code MIT licensed (see LICENSE.md). Korg code BSD 3-Clause Licensed, license headers retained in relevant code files.

