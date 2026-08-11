#pragma once

typedef struct
{
    float threshold0, ratio0;   // Compress remainder by ratio0 if signal > threshold0
    float threshold1, ratio1;   // Compress remainder by ratio1 if signal < threshold1
} AsymComprParams;

float process_asymcompr(const AsymComprParams * p, float input);
