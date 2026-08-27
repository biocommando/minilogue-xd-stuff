## Electric guitar
Electric guitar oscillator that tries to produce clean guitar tone as
a basis for a distorted sound (so the idea is not to produce the best
clean guitar sound). Works by having a sampled guitar waveform that is
decayed away while a triangle wave waveform kicks in. Can use an
optional additional noise transient. Uses 2 oscillators: either in 7 or 12
semitones interval.

### Parameters

#### Shape parameters
- Shape:
    * Tone: left side = lowpass, right side = high pass
- Shift + Shape:
    * Distortion. Ideally, use a separate distortion model at the end of
    the processing chain.

#### User parameters
- 1: Attack length (waveform mix transition speed)
- 2: Interval: 1 = 7, 2 = 12 semitones
- 3: Low-passed noise transient mix
- 4: Overall volume decay (before distortion)
