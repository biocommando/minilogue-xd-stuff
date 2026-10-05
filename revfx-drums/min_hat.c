#include "dataoscsrc.h"
// from file /mnt/shared/minimal_hat.wav

static const int8_t _data[72] = {
   -127, 17,-82, 83, 23, -4,-36,102, 83, 65,-73, 92, 57, 13,-33,-95,-75,-13,-58,-52,117,  7,-56,-77,-75, -5, 21, 27, 30, 29,-45, 34, -2,-19,-64, 15, 39, 39, -1,-31,
    41, 20, 47, 41, -3,-47,-12,-34, 28,-34, 19, 24, 36, 35,  5,-10,-17,-14, 19,-27, -6,-19,  8,-17,-17, 15, -5, -4,  3,  4,  6,  3,
};

struct waveform_data get_min_hat_waveform()
{
    struct waveform_data wfd;
    wfd.data = _data;
    wfd.length = sizeof(_data);
    return wfd;
}
