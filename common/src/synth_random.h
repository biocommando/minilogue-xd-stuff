#pragma once

// Return a pseudo random number
unsigned synth_random();

// Reseed with value or default value if 0
void synth_random_reset(unsigned value);

// Get random number in range 0...1
float synth_randomf();

// Get random number in range -1...1
float synth_random_noise();
