#include "dataoscsrc.h"
// from file /mnt/shared/minimal_kick.wav

static const int8_t _data[74] = {
    13, 25, 37, 49, 60, 70, 80, 88, 96,103,109,114,119,122,124,126,127,126,125,123,121,117,113,108,103, 98, 91, 85, 78, 71, 63, 56, 49, 41, 34, 26, 19, 12,  6,
     0, -5,-11,-16,-21,-25,-29,-32,-35,-37,-38,-40,-40,-41,-40,-40,-39,-37,-36,-34,-32,-29,-27,-24,-21,-19,-16,-13,-11, -8, -6, -4, -2, -1,
};

struct waveform_data get_min_kick_waveform()
{
    struct waveform_data wfd;
    wfd.data = _data;
    wfd.length = sizeof(_data);
    return wfd;
}
