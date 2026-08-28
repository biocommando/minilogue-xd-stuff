# Supersaw oscillator
Stack of 7 to 13 saw oscillators.

- Shape controls spread
- Shift + shape controls pitch drift per oscillator.

Has user parameters:
- 1: Num oscs
	- Number of oscillators. Mapped values:
		- 1 = 7 oscs
		- 2 = 9 oscs
		- 3 = 11 oscs
		- 4 = 13 oscs
- 2: Attenuation
	- Total signal attenuation. Use this to prevent clipping within the
	  user oscillator. The output path has a soft clip distortion to
	  soften the clipped sound but most often the distorted sound is not
	  desired.
- 3: Gate pttrn (gate pattern):
	- Controls the pattern of a trance gate. The gate is controlled by
	  shape LFO, and the gated state volume is controlled by the LFO
	  intensity.
- 4: Amp dist (amplitude distribution):
	- at 0 all oscillators play at equal amplitude
	- at 100 only center oscillator plays
- 5: Ph rand mod (phase randomizer modifier):
	- modifies how much the phase is randomized at note-on
    - 0 = full random, 100 = no random
- 6: HPF cutoff
	- 1-pole highpass that's applied before soft clipping. range 0-1.5 kHz
