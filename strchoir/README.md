## String and choir with piano
A sample-playback user oscillator that has looping strings and choir samples and a one-short piano sample layered together.
This is supposed to be a more sophisticated implementation of the idea behind the "piano" oscillator that was my first user
oscillator, with cleaner looping layered samples and more predictable parameters.

### Parameters

#### Shape parameters
- Shape:
    * Mix ratio between strings and choir (0 = 100 % strings, 1 = 100 % choir) 
- Shift + Shape:
    * Piano sample mix ratio. At 100 % only piano sample will play.

#### User parameters
- 1: Detune
- 2: Piano sample octave (range -1...+2)
- 3: Duck piano (1=off, 2=on)
    * Add a short attack envelope to strings/choir samples
- 4: Piano waveform (1=piano, 2=glockenspiel)
- 5: Tape effect
    * Mellotron inspired tape effect intensity
- 6: Granule size
    * If non-zero, will cause random phase jumps at the selected interval

