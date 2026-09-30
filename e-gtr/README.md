## Electric guitar
Electric guitar oscillator that tries to produce clean guitar tone as
a basis for a distorted sound (so the idea is not to produce the best
clean guitar sound). Works by having a sampled guitar waveform that is
decayed away while a triangle wave waveform kicks in. Can use an
optional additional noise transient. Uses 2 oscillators with selectable
interval.

### Parameters

#### Shape parameters
- Shape:
    * Tone: left side = lowpass, right side = high pass
    * Note that the shape LFO modulates the mix between sampled guitar and triangle waveform
- Shift + Shape:
    * Distortion. Ideally, use a separate distortion model at the end of
    the processing chain.

#### User parameters
- 1: Attack length (waveform mix transition speed)
- 2: Interval: 1 = 7 semitones, 2 = 12 semitones, 3 = 5 semitones, 4 = slight detune
- 3: Low-passed noise transient mix
- 4: Overall volume decay (before distortion)
- 5: Strum delay (0..66 ms), delay after which the second oscillator is triggered
- 6: "Chirp" modulation amount; modulates the pitch with an audio-range LFO for a short amount in the beginning of a note
