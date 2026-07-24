static unsigned state = 123;

unsigned synth_random()
{
    state = (state >> 5) ^ state;
    state = (state << 7) ^ state;
    return state;
}

void synth_random_reset()
{
    state = 123;
}