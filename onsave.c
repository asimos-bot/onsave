#include <stdlib.h>
#include <sys/inotify.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>

#define MIN(a, b) a < b ? a : b;
#define FLAGS IN_DELETE | IN_DELETE_SELF | IN_MOVE_SELF | IN_MOVE | IN_CREATE | IN_MODIFY | IN_ATTRIB
#define ONCE_FLAG 1 // 0b00000001
#define HELP_FLAG 2 // 0b00000010
#define GIT_FLAG 4 // 0b00000100
#define IGNORE_FLAG 8 // 0b00001000
#define CONFIG_FLAG 16 // 0b00010000
#define ERROR_FLAG 32 // 0b00100000

typedef struct ONSAVE_CONFIG {
    uint8_t flags;
    unsigned int target_idx; // command start idx is this + 1
    unsigned int config_idx;
    unsigned int ignore_len;
    unsigned int* ignore_list;
} ONSAVE_CONFIG;

void help() {
    printf("onsave [OPTIONS] [FILE|DIRECTORY] [COMMAND...]\n");
    printf("-o / --once   - run only once\n");
    printf("-h / --help   - show this help menu\n");
    printf("-g / --git    - use only git tracked files\n");
    printf("-i / --ignore - ignore this file/directory\n");
    printf("-c / --config - read options from config file (multiple config files not supported)\n");
}

uint8_t strequal(char* a, char* b, unsigned int len) {
    for(unsigned int i = 0; i < len; i++) {
        if(a[i] != b[i]) return 0;
    }
    return 1;
}

#define SET_BIT_AND_CONTINUE(var, bit) var |= bit; continue;

void parse_args(unsigned int argc, char** argv, ONSAVE_CONFIG* config) {
    uint8_t next_is_parameter = 0;
    for(unsigned int arg_counter = 1; arg_counter < argc; arg_counter++) {
        char* arg = argv[arg_counter];
        // not argument
        if(arg[0] != '-') {
            // arg list end
            if(!next_is_parameter) {
                if(arg_counter == argc - 1) config->flags |= ERROR_FLAG;
                config->target_idx = arg_counter;
                return;
            // parameter
            } else {
                switch (next_is_parameter) {
                    case IGNORE_FLAG: {
                        config->ignore_list = realloc(config->ignore_list, ++config->ignore_len);
                        config->ignore_list[config->ignore_len - 1] = arg_counter;
                        break;
                    }
                    case CONFIG_FLAG: {
                        config->config_idx = arg_counter;
                        break;
                    }
                }
            }
        } else {
            uint8_t long_name_offset = arg[1] == '-';
            switch(arg[1 + long_name_offset]) {
                case 'h': { // help
                    SET_BIT_AND_CONTINUE(config->flags, HELP_FLAG);
                }
                case 'o': { // once
                    SET_BIT_AND_CONTINUE(config->flags, ONCE_FLAG);
                }
                case 'g': { // git
                    SET_BIT_AND_CONTINUE(config->flags, GIT_FLAG);
                }
                case 'i': { // ignore
                    SET_BIT_AND_CONTINUE(next_is_parameter, IGNORE_FLAG);
                }
                case 'c': { // config
                    SET_BIT_AND_CONTINUE(next_is_parameter, CONFIG_FLAG);
                }
            }
        }
        next_is_parameter = 0;
    }
}

int main(int argc, char** argv) {

    // check if there is enough arguments
    if(argc < 3) {
        help();
        exit(EXIT_FAILURE);
    }
    // get arguments
    ONSAVE_CONFIG* config = malloc(sizeof(ONSAVE_CONFIG));
    for(unsigned int i = 0; i < sizeof(ONSAVE_CONFIG); i++) ((uint8_t*)config)[i] = 0x00;
    parse_args(argc, argv, config);

    // throw parsing error
    if(config->flags & ERROR_FLAG || config->flags & HELP_FLAG) {
        help();
        exit(config->flags & ERROR_FLAG ? EXIT_FAILURE : EXIT_SUCCESS);
    }

    // initialize inotify_init
    int fd = inotify_init();
    if( fd == -1 ) {
        perror("inotify_init");
        exit(EXIT_FAILURE);
    }

    // watch file
    char* filename = argv[config->target_idx];
    printf("filename: %s\n", filename);
    int wd = inotify_add_watch(fd, filename, FLAGS);

    char buf[4096] = {0};
    ssize_t len = read(fd, buf, sizeof(buf));
    const struct inotify_event *event;
    event = (struct inotify_event*) buf;

    return 0;
}
