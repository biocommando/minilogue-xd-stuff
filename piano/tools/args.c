#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "args.h"

int get_arg(int argc, char *argv[], const char *arg)
{
    for (int i = 0; i < argc - 1; i++)
    {
        if (!strcmp(argv[i], arg))
        {
            return i + 1;
        }
    }
    return -1;
}

void parse_format_config(int argc, char *argv[], struct format_config *config)
{
    int config_arg = get_arg(argc, argv, "-c");
    FILE *f = fopen(config_arg == -1 ? "default.conf" : argv[config_arg], "r");
    if (f == NULL)
    {
        printf("Could not open config file\n");
        exit(1);
    }
    char key[100];
    int value;
    while (fscanf(f, "%s %d", key, &value) != EOF)
    {
        if (!strcmp(key, "stereo"))
        {
            config->stereo = value;
        }
        else if (!strcmp(key, "bits"))
        {
            config->bitdepth = value;
        }
        else if (!strcmp(key, "rate"))
        {
            config->samplerate = value;
        }
    }
}