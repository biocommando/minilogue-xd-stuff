/*
    The idea of this program is to output wavefiles in multiple wav formats.
    One can play the generated files back using a 3rd party program to verify
    the functionality of the library.
*/

#include "wav_handler_cpp.h"
#include <iostream>
#include <cmath>

using namespace wav_handler;

#define assert_that(condition) \
    (!(condition) ? (std::cout << "Assert failed: " << #condition << '\n'), false : true)

void test_load_headers()
{
    WaveFile f;
    f.load_file("sine.dat", {"fact", "PEAK"});
    const unsigned char fact[] = {0x34, 0x3, 0, 0};
    assert_that(f.get_header("fact") == std::string(reinterpret_cast<const char *>(fact), sizeof(fact)));
    const unsigned char peak[] = {0x1, 0x0, 0x0, 0x0, 0xee, 0x71, 0x12, 0x62, 0xae, 0xcc, 0x4c, 0x3f,
                                  0xaa, 0x1, 0x0, 0x0};
    assert_that(f.get_header("PEAK") == std::string(reinterpret_cast<const char *>(peak), sizeof(peak)));
}

void test_write_mono_file()
{
    WaveFile f;
    f.create(44100 * 2, false, 16, 44100, false);
    for (int i = 0; i < 44100 * 2; i++)
        f.set_sample(i, sin(i / 30.0));
    f.write_file("cpp_16bit_44100Hz_int_mono.wav");
}

void test_write_stereo_file()
{
    WaveFile f;
    f.create(44100 * 2, true, 16, 44100, false);
    for (int i = 0; i < 44100 * 2; i++)
        f.set_sample_stereo(i, {sinf(i / 30.0), -sinf(i / 30.0)});
    f.write_file("cpp_16bit_44100Hz_int_stereo.wav");
}

void test_read_mono_file()
{
    WaveFile f;
    f.load_file("cpp_16bit_44100Hz_int_mono.wav");
    assert_that(f.get_length() == 44100 * 2);
    for (int i = 0; i < 44100 * 2; i++)
    {
        if (!assert_that(fabs(f.get_sample(i) - sin(i / 30.0)) < 1e-4))
            break;
    }
}

void test_read_stereo_file()
{
    WaveFile f;
    f.load_file("cpp_16bit_44100Hz_int_stereo.wav");
    assert_that(f.get_length() == 44100 * 2);
    for (int i = 0; i < 44100 * 2; i++)
    {
        const auto s = f.get_sample_stereo(i);
        if (!assert_that(fabs(s.ch0 - sinf(i / 30.0)) < 1e-4) ||
            !assert_that(fabs(s.ch1 - (-sinf(i / 30.0))) < 1e-4))
            break;
    }
}

void test_write_read_headers()
{
    WaveFile f;
    f.create(100, false, 16, 44100, false);
    f.write_file("cpp_header_test.wav", {{"MyH1", "hello world"}, {"MyH2", "another header"}});
    WaveFile f2;
    f2.load_file("cpp_header_test.wav", {"MyH1", "MyH2"});
    assert_that(f2.get_header("MyH1") == "hello world");
    assert_that(f2.get_header("MyH2") == "another header");
}

#define RUN(test)                \
    std::cout << "Running test " << #test << '\n'; \
    test()

int main(int argc, char **argv)
{
    RUN(test_load_headers);
    RUN(test_write_mono_file);
    RUN(test_write_stereo_file);
    RUN(test_read_mono_file);
    RUN(test_read_stereo_file);
    RUN(test_write_read_headers);
}