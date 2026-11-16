#pragma once

static inline int16_t to_signed_12bitval(int16_t v)
{
    return ((int16_t)(v << 4)) >> 4;
}

static inline void get2_from_12bit_buf(int16_t *s, uint32_t pos, const uint8_t *buf)
{
    const uint32_t buf_idx = pos * 3;
    const uint32_t v32bit = (*(uint32_t*)(buf + buf_idx)) & 0x00ffffff;
    *s = ((v32bit >> 12) & 0xfff);
    *(s + 1) = (v32bit & 0xfff);
}

static inline void set2_to_12bit_buf(const int16_t *s, uint32_t pos, uint8_t *buf)
{
    const uint32_t buf_idx = pos * 3;
    uint32_t *v32bit = (uint32_t*)&buf[buf_idx];
    uint32_t v = *(s + 1) & 0xfff;
    v |= ((*s & 0xfff) << 12);
    *v32bit &= 0xff000000;
    *v32bit |= v;
}
