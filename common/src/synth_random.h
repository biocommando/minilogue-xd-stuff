#pragma once

// Return a pseudo random number
unsigned synth_random();

// Reseed with value or default value if 0
void synth_random_reset(unsigned value);
