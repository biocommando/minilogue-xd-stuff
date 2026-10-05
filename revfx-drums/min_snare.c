#include "dataoscsrc.h"
// from file /mnt/shared/minimal_snare.wav

static const int8_t _data[75] = {
   127,126,125,125,123,121,119,118,117,115,113,112,110,108,106,104,102, 99, 97, 94, 92, 90, 87, 84, 82, 80, 80, 80, 80, 80, 80, 67, 28,  7, 38, 73, 82,101, 60, 74,
   114, 97,102, 84, 54, 20, 51, 81,101, 70, 54, 17,-20, -7, 19, 23, 18,  9, -8,-11,-11,-41,-57,-36,-52,-23, 15, 47, 66, 48, 86, 52, 14, 38, 40,
};

struct waveform_data get_min_snare_waveform()
{
    struct waveform_data wfd;
    wfd.data = _data;
    wfd.length = sizeof(_data);
    return wfd;
}
