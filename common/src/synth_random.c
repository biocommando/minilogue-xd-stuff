static unsigned state = 123;

unsigned synth_random()
{
    state = (state >> 5) ^ state;
    state = (state << 7) ^ state;
    return state;
}

void synth_random_reset(unsigned value)
{
    state = value ? value : 123;
}

float synth_randomf()
{
    return synth_random() / (float)0xFFFFFFFF;
}

// Get random number in range -1...1
float synth_random_noise()
{
    return synth_random() / (float)0x7FFFFFFF - 1;
}
