## Korg logue SDK: Collection of user oscillators and effects
User oscillators and effects for the **Korg Minilogue XD** multi-engine, built using the official C/C++ **logue-sdk**.

## Building / binary distribution
A compiled and tested binary of each effect comes with this repository. To build, use the **Minilogue SDK**. To use the same workflow as I, use the legacy building method and place this repository right under `logue-sdk/platform/minilogue-xd`. You can run `make` from within each of the project directories.

You can use the `common/env` script for command-line tools when working with the projects. Source the file to use the `uu` prefixed
commands for managing project building and distribution package copy operations.

## Code organization
There are common utilities in `common/` and all other directories are project directories for individual oscillators/effects.

Each user oscillator / effect is organized like this:
```
user-osc-fx/
    build/          Build directory
    dist/           Contains the latest mnlgxdunit binary that can be loaded
                    to Minilogue XD unit.
    ld/             Logue SDK specifics
    tpl/            Logue SDK specifics
    *.c/*.h         Source files
    Makefile        Makefile
    project.mk      Included by Makefile, contains pointers to user sources
    manifest.json   Packaging metadata
    README.md       Individual oscillator / effect specific documentation
```

## License
Original code MIT licensed (see LICENSE.md). Korg code BSD 3-Clause Licensed, license headers retained in relevant code files.

