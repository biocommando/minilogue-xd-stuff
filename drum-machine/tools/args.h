#pragma once

struct format_config
{
    int stereo;
    int bitdepth;
    int samplerate;
};

void parse_format_config(int argc, char *argv[], struct format_config *config);

int get_arg(int argc, char *argv[], const char *arg);

#define GET_ARG_OPT(name, flag, is_optional)            \
    int name = get_arg(argc, argv, flag);               \
    if (name == -1 && !is_optional)                     \
    {                                                   \
        printf("No " #name " (" #flag ") specified\n"); \
        return 1;                                       \
    }

#define GET_ARG(name, flag) \
    GET_ARG_OPT(name, flag, 0)
