## Korg logue SDK: Collection of user oscillators and effects
User oscillators and effects for the **Korg Minilogue XD** multi-engine, built using the official C/C++ **logue-sdk**.

## Building / binary distribution
A compiled and tested binary of each effect comes with this repository. To build, use the **Minilogue SDK**. To use the same workflow as I, use the legacy building method and place this repository right under `logue-sdk/platform/minilogue-xd`. You can run `make` from within each of the project directories.

## Code organization
There are common utilities in `common/` and all other directories are project directories for individual oscillators/effects. 

## License
Original code MIT licensed (see LICENSE.md). Korg code BSD 3-Clause Licensed, license headers retained in relevant code files.

