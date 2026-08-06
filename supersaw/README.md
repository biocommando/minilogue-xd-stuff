# Supersaw oscillator
Stack of 7 saw oscillators. Shape controls spread, shift shape pitch
drift per oscillator. Has parameters:
- Num oscs
	- Number of oscillators. Mapped values:
		- 1 = 7 oscs
		- 2 = 9 oscs
		- 3 = 5 oscs
		- 4 = 3 oscs
- Amp dist (amplitude distribution):
	- at 0 all oscillators play at equal amplitude
	- at 100 only center oscillator plays
- Ph rand mod (phase randomizer modifier):
	- modifies how much the phase is randomized at note-on
    - 0 = full random, 100 = no random
- Attenuation
	- Total signal attenuation. Use this to prevent clipping within the
	  user oscillator.
