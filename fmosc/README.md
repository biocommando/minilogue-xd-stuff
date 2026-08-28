# 2 OP FM Vector Synth

## Architecture
The oscillator has 2 identical FM synth engines with the following operator architecture:
```
operator 2 --> operator 1 -> output
           /
envelope -/
```

So, `operator 2` modulates `operator 1` with a configurable modulation amount. The modulation amount can be modulated using a simple exponential envelope.

The shape parameter changes the synth engine mix and shift+shape overtone content for operator waveform.

You can modify the following parameters (see below for more details): operator 1 / 2 frequency ratio, modulation amount, modulation envelope length.

### Parameters

#### Shape parameters
- Shape:
    * Synth engine 1 and 2 mix. Minimum value = only play engine 1, maximum value = only play engine 2. 
- Shift + Shape:
    * Harmonic content for operator waveform. At minimum, uses pure sine.

#### User parameters
Synth engine 1 is edited using parameter 1..3 and synth engine 2 using identical parameters 4..6. 

- 1: `S1 Ratio`:
    * Frequency ratio between operators 1 and 2, divided by 8. E.g. 16 = operator 2 frequency is 2 times the base frequency. 
- 2: `S1 ModAmt`:
    * Amount of frequency modulation from operator 2 to operator 1.
    * 0%: Default amount. Use 1% for no modulation.
- 3: `S1 Env`, `S2 Env` -100...100:
    * Envelope length that modulates the modulation amount.
    * 0 = no envelope modulation
    * over 0 = decay envelope
    * below 0 = attack+decay envelope
    * Maximum envelope time ~6 seconds.
- 4: `S2 Ratio`: Similar to `S1 Ratio`
- 5: `S1 ModAmt`: Similar to `S2 ModAmt`
- 6: `S1 Env` similar to `S2 Env`
