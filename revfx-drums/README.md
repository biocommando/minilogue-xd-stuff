## Drum machine (for reverb slot)
A drum machine intended for practice helper. Similar but higher quality
than modfx-drums (reverb effects have more resources to spend).
Uses the 8-bit/16kHz samples for kick, snare, closed hihat and rimshot.
Has three kits: acoustic kit, electronic kit and lo-fi glitch kit.
The drum patterns are based on a 16-step sequencer that also has per-step
modifiers (accent, altered decay, altered pitch, sample playback offset)
that make the drums sound a bit less static.

The beat is synced to tempo. The playback features start/stop/trigger on
input via knob macros.

In addition to the drum machine, the effect contains a looper that
allows recording up to 2 bar audio loop that is synchronized
to the drum beat. The recorded audio is in 16-bit / 48 kHz format.

- TIME parameter controls the looper playback volume
  * When volume is set to 0, the looper will start recording. When volume
    is set to non-zero, the looper is played back. The loop time will be
    always 2 bars. When in record mode, a low beep will indicate the start
    of the loop.
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
    If you twist the SHIFT + DEPTH knob while paused in input trigger monitoring mode,
    it will switch the used drum kit. Use clockwise/counter-clockwise twist for different kits.
    If you twist the TIME know while paused in input trigger monitoring mode,
    it will switch between loop replace/overdup modes.
    If TIME parameter is 0 when entering input trigger monitoring mode, the looper
    will record only once and then enter play mode.
- SHIFT + DEPTH parameter selects the pattern and controls the looper. Pattern
  change is indicated by playing back a short beep sound, and a lower beep
  sound is used for 2-bar patterns. Patterns:
  * 0 non-accented metronome -- 1 bar
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
