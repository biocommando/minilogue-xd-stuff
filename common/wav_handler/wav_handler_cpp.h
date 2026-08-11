#pragma once
#include <string>
#include <stdexcept>
#include <map>
#include <tuple>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "wav_handler.h"

namespace wav_handler
{
    /**
     * @brief Struct for reading and writing stereo samples
     */
    struct StereoSample
    {
        /**
         * @brief First channel sample (left)
         */
        float ch0;
        /**
         * @brief Second channel sample (right)
         */
        float ch1;
    };

    class WaveFile
    {
        wav_file wav;
        std::map<std::string, std::string> custom_header_data;

        WaveFile(const WaveFile &) = delete;
        WaveFile &operator=(const WaveFile &) = delete;
        WaveFile(WaveFile &&) = delete;
        WaveFile &operator=(WaveFile &&) = delete;

    public:
        WaveFile()
        {
            memset(&wav, 0, sizeof(wav_file));
        }

        ~WaveFile()
        {
            if (wav.data)
            {
                free_wav_file(&wav);
            }
        }

        /**
         * @brief Load a wave file.
         *
         * Throws exception if data is already initialized or there's an error while reading the file.
         *
         * @param fname The filename.
         * @param custom_headers Custom header names to be loaded.
         */
        void load_file(const std::string &fname, const std::vector<std::string> &custom_headers = {})
        {
            if (wav.data)
                throw std::runtime_error("file already loaded");
            std::vector<wav_file_custom_header_data> chdr_array;
            int i = 0;
            for (auto &hdr : custom_headers)
            {
                chdr_array.push_back({});
                chdr_array[i].data = nullptr;
                strcpy(chdr_array[i].header_name, hdr.substr(0, 4).c_str());
                chdr_array[i].num_bytes = 0;
                i++;
            }
            chdr_array.push_back({{0}, 0, nullptr});
            const auto err = read_wav_file_chdr(fname.c_str(), &wav, chdr_array.data());
            for (auto &hdr : chdr_array)
            {
                if (hdr.data)
                    custom_header_data[hdr.header_name] = std::string(hdr.data, hdr.num_bytes);
                free(hdr.data);
            }
            if (err)
                throw std::runtime_error("Error loading file" + fname);
            if (wav.channels > 16)
                throw std::range_error("Number of channels must be max 16");
        }

        /**
         * @brief Write the wave data to a file.
         *
         * Throws exception if data hasn't been initialized or there's an error while writing the file.
         *
         * @param fname The file name.
         * @param custom_headers Custom headers to be saved as a map where the key is the custom header
         * name and the value is the custom header value.
         */
        void write_file(const std::string &fname, const std::map<std::string, std::string> custom_headers = {}) const
        {
            if (!wav.data)
                throw std::runtime_error("no file loaded");

            std::vector<wav_file_custom_header_data> chdr_array;
            int i = 0;
            for (auto &hdr : custom_headers)
            {
                chdr_array.push_back({});
                chdr_array[i].data = static_cast<char *>(calloc(hdr.second.size(), 1));
                memcpy(chdr_array[i].data, hdr.second.data(), hdr.second.size());
                strcpy(chdr_array[i].header_name, hdr.first.substr(0, 4).c_str());
                chdr_array[i].num_bytes = hdr.second.size();
                i++;
            }
            chdr_array.push_back({{0}, 0, nullptr});
            int err = write_wav_file_chdr(fname.c_str(), &wav, chdr_array.data());
            for (auto &hdr : chdr_array)
            {
                free(hdr.data);
            }
            if (err)
                throw std::runtime_error("Error writing file" + fname);
        }

        /**
         * @brief Create an empty all-zeroes wave file.
         *
         * @param length Length in mono/stereo samples
         * @param stereo True if stereo, false if mono
         * @param bit_depth Bit depth in bits. See wav_get_normalized documentation for list of supported formats.
         * @param sample_rate Sample rate (Hz)
         * @param is_float True if floating point format is used
         */
        void create(unsigned length, bool stereo, unsigned bit_depth, unsigned sample_rate, bool is_float)
        {
            if (wav.data)
                throw std::runtime_error("file already loaded");
            auto err = create_wav_file(&wav, length, stereo ? 2 : 1, bit_depth, sample_rate);
            if (err)
                throw std::runtime_error("Error creating wav file");
            wav.is_float = is_float;
        }

        /**
         * @brief Get a custom header by name. Throws an invalid_argument exception if
         * header is not found. Returns the header value as string.
         */
        std::string get_header(const std::string &name) const
        {
            auto data = custom_header_data.find(name);
            if (data == custom_header_data.end())
                throw std::invalid_argument("Header not found: " + name);
            return data->second;
        }

        /**
         * @brief Get the length of data in mono/stereo samples.
         * Throws runtime_error if data is not initialized.
         */
        unsigned get_length() const
        {
            if (!wav.data)
                throw std::runtime_error("no file loaded");
            return wav.num_frames;
        }

        /**
         * @brief Get a mono sample from index idx. If the data is in stereo, returns the sample from channel 0.
         * Throws runtime_error if data is not initialized or if getting the sample fails.
         */
        float get_sample(unsigned idx) const
        {
            if (!wav.data)
                throw std::runtime_error("no file loaded");
            float sample_buf[16];
            int err = wav_get_normalized(&wav, idx, sample_buf);
            if (err)
                throw std::runtime_error("error while getting sample at index " + std::to_string(idx));
            return sample_buf[0];
        }

        /**
         * @brief Get a stereo sample from index idx. If the data is in mono, the contents of the second channel
         * will be same as the first channel.
         * Throws runtime_error if data is not initialized or if getting the sample fails.
         */
        StereoSample get_sample_stereo(unsigned idx) const
        {
            if (!wav.data)
                throw std::runtime_error("no file loaded");
            float sample_buf[16];
            int err = wav_get_normalized(&wav, idx, sample_buf);
            if (err)
                throw std::runtime_error("error while getting sample at index " + std::to_string(idx));
            if (wav.channels == 1)
                sample_buf[1] = sample_buf[0];
            return {sample_buf[0], sample_buf[1]};
        }

        /**
         * @brief Set a mono sample at index idx. If the data is in stereo, sets both channels to the sample.
         * Throws runtime_error if data is not initialized or if setting the sample fails.
         */
        void set_sample(unsigned idx, float value)
        {
            if (!wav.data)
                throw std::runtime_error("no file loaded");
            float sample_buf[2] = {value, value};
            int err = wav_set_normalized(&wav, idx, sample_buf);
            if (err)
                throw std::runtime_error("error while setting sample at index " + std::to_string(idx));
        }

        /**
         * @brief Set a stereo sample at index idx. If the data is in mono, only data from channel 0 is used.
         * Throws runtime_error if data is not initialized or if setting the sample fails.
         */
        void set_sample_stereo(unsigned idx, const StereoSample &value)
        {
            if (!wav.data)
                throw std::runtime_error("no file loaded");
            float sample_buf[2] = {value.ch0, value.ch1};
            int err = wav_set_normalized(&wav, idx, sample_buf);
            if (err)
                throw std::runtime_error("error while setting sample at index " + std::to_string(idx));
        }

        /**
         * @brief Returns true if the data is in stereo.
         * Throws runtime_error if data is not initialized.
         */
        bool is_stereo() const
        {
            if (!wav.data)
                throw std::runtime_error("no file loaded");
            return wav.channels > 1;
        }
    };

}