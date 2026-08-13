## Tube Overdrive
This is a tube distortion that occupies the modulation effect slot.
Has asymmetric compression at both input and output stages and the distortion
algorithm is based on square root function. Uses 2 iteration Halley's method
with some smart initial value guessing for square root approximation.

Has a fixed-level noise gate that is applied after post-gain, so the post-gain
both controls the output level and noise gating.

- TIME parameter controls the pre-gain.
- DEPTH parameter controls the post-gain.
