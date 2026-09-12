# Sampler oscillator
This is an oscillator that can play back a custom 8-bit sample.
The sample data is sent using a custom protocol via the shift-shape parameter
that is MIDI controllable. There's an HTML based data uploader
and also a program for transforming wav files into JSON format that can be
sent using the uploader.

## Protocol
- Bits 0...3: Data
- Bits 4...6: Meta
META:
- 0..3 -> sequence number
- 4 -> start data segment. DATA contains id:
  * 1: samplerate
  * 2: base frequency
  * 3: loop index
  * 4: data
  * others: nothing selected
- others: state machine reset

Data segments:
DATA contains the words in big-endian format (so first message contains
most significant 4 bits), split into 4-bit chunks.

- 1: samplerate as 16 bit unsigned integer
- 2: base frequency as 32 bit unsigned integer. The hertz value is obtained
     by dividing the received number by 1e6.
- 3: loop index (where playback is reset when the sample ends) as 16
     bit unsigned integer
- 4: sample data as 8-bit signed integers. The data is stored so that
     if the word > 127, the actual value is 127 - word.

When sending the data segments, the META word contains the "sequence
number" which is a rolling number from 0 to 3. It's used for checking
when the received data is complete.

## Parameters
- Shift + Shape: Reserved for sample data entry 
