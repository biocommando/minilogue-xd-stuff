# Sampler oscillator
This is an oscillator that can play back a custom 8/16-bit sample.
The sample data is sent using a custom protocol via the shift-shape parameter
that is MIDI controllable. There's an HTML based data uploader
and also a program for transforming wav files into JSON format that can be
sent using the uploader.

## Protocol
- Bits 0...3: DATA
- Bits 4...6: META

META:

- 0..3 -> sequence number
- 4 -> start data segment. DATA contains id (see segments.h):
  * SEG_SAMPLERATE: samplerate
  * SEG_BASEFREQ: base frequency
  * SEG_LOOPIDX: loop index
  * SEG_BITS: bitdepth
  * SEG_WAVE: data
  * others: nothing selected
- others: state machine reset

Data segments:

DATA contains the words in big-endian format (so first message contains
most significant 4 bits), split into 4-bit chunks.

- SEG_SAMPLERATE: samplerate as 16 bit unsigned integer
- SEG_BASEFREQ: base frequency as 32 bit unsigned integer. The hertz value is obtained
				by dividing the received number by 1e6.
- SEG_LOOPIDX: loop index (where playback is reset when the sample ends) as 16
			   bit unsigned integer
- SEG_BITS: bitdepth as 8 bit unsigned integer. Values 8 and 16 supported.
- SEG_WAVE: sample data as 8 or 16 bit signed integers. The data is stored so that
			if the word > max_positive_range, the actual value is max_positive_range - word.

When sending the data segments, the META word contains the "sequence
number" which is a rolling number from 0 to 3. It's used for checking
when the received data is complete.

## Parameters
- Shape: Chorus amount
- Shift + Shape: Reserved for sample data entry

- 1: Split
	* Split sample start times into 4 regions that can be triggered from
	  different keys on the keyboard. 25% corresponds to 4 equally spaced
	  regions.
- 2: Track kb
	* Pitch follows keyboard if set to 1. If set to 2, plays at constant speed.
- 3: Retrig
	* Retrigger after a delay. 100% = 1 second
- 4: Retrig amp
	* Modify amplitude of the delayed retriggered signal. These parameters
	allow adding a midi-delay type of effect.
- 5: Interpolation.
	* 1 = linear
	* 2 = no interpolation
- 6: Reverse
	* 1 = No reverse
	* 2 = Reverse play direction
