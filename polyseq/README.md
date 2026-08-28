# Polyphonic sequencer oscillator
This is an oscillator that can play back a 4-voice polyphonic sequence.
The sequence is sent using a custom protocol via the shift-shape parameter
that is MIDI controllable. There's an HTML based data uploader under editor/
and also a program for transforming MIDI files into JSON format that can be
sent using the uploader. The sequence supports 2048 individual note events
(note on/note off).

## Protocol
Code level documentation found in source code, here only the implementation
in editor/seq.js documented.

- 0, 127, 0, 127, 0, 127: Start sequence
- Any number of notes:
    - 1 or 2..126: Note on event if byte > 1, note off if byte = 1
    - 2..126: Only if previous byte was 1, note off note value
    - up to 5 delta words (sample offset to the next event):
        - 126: Sent only if next byte would be same as previous 
        - 1..67: 5 bytes value shifted left by 1. At each new delta word
                 the previous delta value is shift left by 5 and added
                 to the new value. As the actual value that will be used
                 is stored in the bits 1..5, a zero can be sent using the
                 left-over 1 LSB.
    - 0: Ends the transmission for the current note
- 127, 0: Sequence end -> start play back

## Parameters
- Shape: playback speed
- Shift + Shape: Reserved for sequence data entry 
- 1: Waveform (saw, square, triangle)
- 2: Decay envelope length
- 3: Split
    - less than 0: notes below this value don't play sound from this oscillator
    - more than 0: notes above this value don't play sound from this oscillator
- 4: Tracking: 1 = keyboard tracking on, 2 = off

