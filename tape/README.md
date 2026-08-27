## Tape emulation
A tape emulation with the following signal flow:
```
input ->
    add hiss & 50 Hz hum ->
        delay line (modulators: wow, noise, random slowdown spikes), speed controlled using a PI controller
            -> compression -> hard clip -> cubic saturation
                -> dynamic LP filter (cuts more the higher the input signal)
```

- TIME parameter controls pitch related effects.
- DEPTH parameter controls distortion and degradation.

## Variants

- tape
	- emulates a low grade cassette that's completely worn out in its extreme settings
- r2reel
	- emulates a more high-end studio-grade reel-to-reel tape machine
