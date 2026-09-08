## Drum machine
A drum machine intended for practice helper.
Uses the same samples and sample playback mechanism as the drum-machine
user oscillator, but only samples kick, snare, closed hihat and rimshot.
The drum patterns are based on a 16-step sequencer that also has per-step
modifiers (accent, altered decay, altered pitch, sample playback offset)
that make the drums sound a bit less static.

The beat is synced to tempo. The playback features start/stop/trigger on
input via knob macros.

In addition to the drum machine, the effect contains a looper that
allows recording up to 5.46 seconds of audio into a loop that is synchronized
to the drum beat. The recorded audio is in 8-bit variable samplerate format
which is very low quality but perfectly usable for jamming. The looper
uses variable samplerate for tempos 87.9 ... 175.8 bpm to allow recording
a full 2-bar loop. For tempos below 87.9 bpm the samplerate is fixed
to 24 kHz which is enough for at least 1-bar loops for the full valid
tempo range and for tempos above 175.8 bpm the samplerate is fixed to
full 48 kHz (the remainded of the buffer will just be unused).

- TIME parameter selects the pattern and controls the looper. Pattern
  change is indicated by playing back a short beep sound, and a lower beep
  sound is used for 2-bar patterns. Patterns:
  * 0 non-accented metronome -- 1 bar
	* Selecting this pattern will stop looper playback.
  * 1 rock 1 -- 2 bars
  * 2 rock 2 -- 2 bars
  * 3 slow beat -- 2 bars
  * 4 4 on the floor -- 2 bars
  * 5 pop 1 -- 2 bars
  * 6 pop 2 -- 1 bar
  * 7 funky rock -- 1 bar
  * 8 pitch var hh beat -- 1 bar
  * 9 fast hats, slow kick snare -- 1 bar
  * 10 straight rock with percs -- 1 bar
  * 11 funky rock with percs -- 1 bar
  * 12 rocky hihats -- 1 bar
  * 13 amen break -- 1 bar
  * 14 funky drummer -- 1 bar
  * 15 metronome 2 -- 1 bar
  * 16 metronome -- 2 bars
	* Selecting this pattern will start recording a loop. If loop time
	  runs out before the beat loops, the recording will be just cut.
	  The loop is played back whenever any of the patterns 1...15 is
	  selected. Note that this pattern is 2 bars so if you use the whole
	  2 bars of recording time, you'll need to select one of the 2-bar
	  patterns to play it back completely (otherwise only half will play).
	  On the other hand, it makes it easier to record perfectly looping
	  1-bar loops because you'll need to immediately switch to another
	  pattern after recording or else the recording will start to overwrite
	  the current record buffer.
- DEPTH parameter controls the mix volume.
  * Setting depth to zero will reset the sequence so it can be used as a
    start/stop control. It also mutes the looper.
  * Setting depth to zero twice (twist x->0->x->0) will arm the input
    trigger monitoring. When depth is set to non-zero after this, playing
    a note from the synth engine will start the sequence. When the
    input trigger monitoring is activated, it's indicated by 3 short
    beeps. If you hear less than 3 beeps, it means that the knob was
    accidentally twisted and it's (possibly) no longer armed.
    When the input trigger monitoring is armed (but not yet active), the
    effect monitors the input level and tries to detect the noise floor.
    If you happen to play notes in this mode, the sequence might not
    trigger at all. If you wait a while before turning the depth knob up,
    the noise floor detection will recover automatically. If the playback
    still doesn't start (e.g. the sequence keeps running during the whole
    procedure), you can recover from this by doing a stop/start and
    then retry the arming.
