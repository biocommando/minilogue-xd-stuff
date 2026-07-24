#ifndef WAV_HANDLER_H
#define WAV_HANDLER_H

/**
 * @brief Data container for a RIFF file 
 */
struct wav_file
{
   /**
    * @brief Total number of bytes in data
    */
   unsigned num_bytes;
   /**
    * @brief Number of channels
    */
   unsigned channels;
   /**
    * @brief Number of n-channel samples.
    * 
    * Equation:
    * 
    * num_frames = num_bytes / (channels * byte_per_sample)
    */
   unsigned num_frames;
   /**
    * @brief Sample rate in Hz
    */
   unsigned sample_rate;
   /**
    * @brief Bit depth in bits
    */
   unsigned bit_depth;
   /**
    * @brief If floating point format is used this is set to 1, 0 otherwise
    */
   int is_float;
   /**
    * @brief The data as a binary blob. Length in num_bytes.
    */
   char *data;
};

/**
 * @brief A structure for reading and writing custom headers.
 * See read_wav_file_chdr and write_wav_file_chdr.
 */
struct wav_file_custom_header_data
{
   /**
    * @brief The header name. Should be always 4 characters + terminating 0.
    */
   char header_name[5];
   /**
    * @brief Number of bytes in data
    */
   unsigned num_bytes;
   /**
    * @brief The header data as a binary blob. Length in num_bytes.
    */
   char *data;
};

/**
 * @brief Read wav file without custom header information. See read_wav_file_chdr for more information.
 */
int read_wav_file(const char *file_name, struct wav_file *wav);

/**
 * @brief Read wav file and populate the wave file given as parameter.
 * 
 * Parameter chdr contains an array of wav_file_custom_header_data. The list must be terminated
 * by an instance where header_name field is empty (i.e. header_name[] = {0}).
 * The list tells the file reader what custom headers should be read.
 * The fields num_bytes and data in list items will be populated if the custom header is found.
 * The logic assumes that data is set to NULL on init.
 * 
 * Returns 0 on success.
 */
int read_wav_file_chdr(const char *file_name, struct wav_file *wav, struct wav_file_custom_header_data *chdr);

/**
 * @brief Free allocated wave file. Files allocated using read_wav_file(_chdr) and create_wav_file must
 * be freed using this function after use.
 * 
 * Returns 0 on success.
 */
int free_wav_file(struct wav_file *wav);

/**
 * @brief Writes wav to file without custom headers. See write_wav_file_chdr for more information.
 */
int write_wav_file(const char *file_name, const struct wav_file *wav);

/**
 * @brief Writes wav to file.
 * 
 * Parameter chdr contains an array of wav_file_custom_header_data. The list must be terminated
 * by an instance where header_name field is empty (i.e. header_name[] = {0}).
 * The list contains all the custom headers that should be written to the file.
 * Custom headers are written before "data" chunk.
 * 
 * Returns 0 on success.
 */
int write_wav_file_chdr(const char *file_name, const struct wav_file *wav, const struct wav_file_custom_header_data *chdr);

/**
 * @brief Creates an all-zeroes wave file with the provided length in n-channel samples, number of channels,
 * bit depth (bits) and sample rate (Hz).
 * 
 * Returns 0 on success.
 */
int create_wav_file(struct wav_file *wav, unsigned num_frames, unsigned channels, unsigned bit_depth, unsigned sample_rate);

/**
 * @brief Get an n-channel sample into value pointer from index sample_idx. The sample is normalized to
 * the range -1.0f ... 1.0f. The value pointer must contain at least as many elements as there are channels.
 * 
 * This function works with the following formats:
 *  8 bit unsigned
 * 16 bit signed
 * 24 bit signed
 * 32 bit float
 * 64 bit float
 * 
 * Returns 0 on success.
 */
int wav_get_normalized(const struct wav_file *wav, unsigned sample_idx, float *value);

/**
 * @brief Set an n-channel sample from value pointer to index sample_idx. The sample should be in range
 * -1.0f ... 1.0f. If the sample is outside of the range the sample is clipped. The value pointer must
 * contain at least as many elements as there are channels.
 * 
 * Same format restrictions apply as in wav_get_normalized.
 * 
 * Returns 0 on success.
 */
int wav_set_normalized(struct wav_file *wav, unsigned sample_idx, float *value);

#endif
