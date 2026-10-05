#include "dataoscsrc.h"
// from file /mnt/shared/minimal_rim.wav

static const int8_t _data[6] = {
   -126,125,-64,64,-32,32,
};

struct waveform_data get_min_rim_waveform()
{
    struct waveform_data wfd;
    wfd.data = _data;
    wfd.length = sizeof(_data);
    return wfd;
}
