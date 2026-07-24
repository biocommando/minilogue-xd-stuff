## Korg logue SDK: Piano/Granular User Oscillator
A sample-playback and granular synthesis user oscillator for the **Korg Minilogue XD** multi-engine, built using the official C/C++ **logue-sdk**.

## Building / binary distribution
A compiled and tested binary comes with this repository. To build, use the **Minilogue SDK**. To use the same workflow as I, use the legacy building method and place this repository right under `logue-sdk/platform/minilogue-xd`.

## Architecture
Based on a simple sample playback engine that has a single ~1.4 seconds long piano note (middle C) sampled from a software instrument with a touch of compression. The data has been compressed to 22050 Hz / 8 bit signed format (source file generated during build, source sample in `sampledata/piano.wav`, see the next chapter for more information).

The sample is played back until the end is reached where the engine loops the tail at the fundamental frequency.

The signal is fed into 1-pole low-pass filter to clean up the lo-fi grittiness.

## Changing source sample

The required sample data is injected to the oscillator via a C source file containing a single large `const int8_t` array. The file is created from a `.wav` file using the included `tools/8bit_dump` tool. The Makefile will first compile the tool using native `gcc` and then execute the `sample_data` target which will run a script like:
```sh
tools/8bit_dump -i sampledata/piano.wav -o generated_dataoscsrc.c -ts 1 -te 0
```

The tool has following parameters:
- `-i`: input wav file. To change the wave data, modify this parameter. The wave file needs to be in `RIFF` format. The sample will be amplified automatically and bitdepth can be whatever, the tool automatically handles conversion to `int8_t`. The samplerate however should be the same as the target samplerate. If the sample is a stereo sample, only the left channel will be used. The code assumes the sample to be a middle C note.
- `-o`: output C source file. Do not modify.
- `-ts`, `-te`: Trim start / end of the wav file. As the bitdepth is reduced to only 8 bits, if the sample starts or ends with a gradual attack, the beginning/end of the transient may be lost. To reduce the need to edit the sample itself, you can just trim the would-be-zero values using the `-ts` and `-te` parameters. The tool will print errors like the following if there are leading or trailing zeroes:
    ```
    Redundant 12 zeroes at start
    Redundant 3 zeroes at end
    ```
    In this example, you would use the trim parameters `-ts 12 -te 3` to remove redundant silence and save some bytes of RAM (although having a couple of trailing zeroes might result in a cool musical effect).

The repository comes with three different samples: piano.wav, guitar.wav and choir.wav. You can switch between each of these using make target `chg_<samplename>`, e.g.:
```sh
make chg_piano
```

These will identify as "piano", "guitar" and "choir" in the manifest.json that is generated from manifest.json.in template.

As far as I have noticed, samples that have an impact type transient will work the best, like piano and guitar samples. If you try the choir sample, it has a cool sound too but the granular/wavetable features don't result in as musically interesting results.

### Parameters
By default the oscillator is in "ROMpler playback mode". You can add in wavetable and granular like features using the **Shape** and **Shift + Shape** parameters, respectively. The user parameters are used for fine tuning these features.

#### Shape parameters
- Shape:
    * Sample length
- Shift + Shape:
    * Bounce point. If not 0, bounces back to this position. Relative to the configured sample length. (E.g. 12 o'clock = bounces back to index sample length / 2)

#### User parameters
- 1: `Loop Mode` 1..2:
    * 1 = after note release, play sample to end and stop
    * 2 = after note release, play sample to end and loop tail
- 2: `Bounce Count` 1..33:
    * 1..32: Before going to tail looping mode, bounce back this many times
    * 33: Infinite bounces
- 3: `Bnc Rst Prob` ("bounce reset probability") 1..64:
    * Probability of bouncing back to beginning of the sample instead of using the configured bounce point
- 4: `BW Limit` 1..40:
    * Bandwidth limiting filter cutoff (times fundamental frequency)
    * Default: 20

## License
Original code MIT licensed (see LICENSE.md). Korg code BSD 3-Clause Licensed, license headers retained in relevant code files.

