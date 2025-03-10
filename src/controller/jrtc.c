// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "jrtc_int.h"
#include "getopt.h"
#include "jrtc_logging.h"
#include "stdlib.h"

int
main(int argc, char* argv[])
{
    char *config_file = NULL;
    
    // Define long options
    struct option long_options[] = {
        {"config", required_argument, 0, 'c'},
        {0, 0, 0, 0} // Terminating NULL entry
    };

    int option_index = 0;
    int opt;
    
    while ((opt = getopt_long(argc, argv, "c:", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'c':
                config_file = optarg;  // Store config filename
                jrtc_logger(JRTC_INFO, "Using config file: %s\n", config_file);
                break;
            case '?':
                fprintf(stderr, "Usage: %s --config=config.yaml\n", argv[0]);
                return -1;
        }
    }
    
    return start_jrtc(config_file);
}
