## Tools for creating waveforms for the drum machine

Make WAV file compression tool `compr` by running `make`.

To create a `.bin` file for C file dumping, use:
```sh
./compr -m r -i input.wav -o output.wav
```

Here `output.wav` will contain the result of the compression process for verifying how it sounds like. It creates a `.bin` file with name `input.wav.bin`. If the output reports redundant zeroes, you can trim the start and end with `-ts NUM` and `-te NUM` arguments respectively. To create a C file that contains the compressed data as a C array, use:
```sh
./compr -m C -i input.wav.bin -o output.c
```

To combine multiple C files into one file with a waveform selector, use `combine.py`, run it like
```sh
combine.py my_file1.c my_file2.c
```

This will create files `combined_waveforms.h` and `combined_waveforms.c`. The filenames will be used as such for id macros (like `WAVEFORM_ID_my_file1`) so they need to be valid C variable names.
