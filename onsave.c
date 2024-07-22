#include <asm-generic/errno.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>

#define MIN(a, b) a < b ? a : b;
#define FLAGS (IN_CLOSE_WRITE)
// #define FLAGS (IN_DELETE | IN_DELETE_SELF | IN_MOVE_SELF | IN_CLOSE_WRITE | IN_MOVE | IN_CREATE | IN_MODIFY | IN_ATTRIB)
#define WATCH_FLAGS FLAGS | IN_MASK_ADD | IN_EXCL_UNLINK
#define ONCE_FLAG 1 // 0b00000001
#define HELP_FLAG 2 // 0b00000010
#define GIT_FLAG 4 // 0b00000100
#define IGNORE_FLAG 8 // 0b00001000
#define VERBOSE_FLAG 16 // 0b00100000
#define QUIET_FLAG 32 // 0b01000000
#define ERROR_FLAG 64 // 0b10000000

typedef struct {
    uint8_t flags;
    unsigned int file_idx; // command start idx is this + 1
    unsigned int ignore_len;
    unsigned int* ignore_arr;
} OnsaveConfig;

void help() {

    printf("onsave [OPTIONS] FILE|DIRECTORY COMMAND\n");
    printf("-o,--once    - run only once\n");
    printf("-h,--help    - show this help menu\n");
    printf("-g,--git     - monitor only git tracked files in the given directory\n");
    printf("-i,--ignore  - ignore this file/directory (relative to given directory)\n");
    printf("-q,--quiet   - no error output\n");
    printf("-v,--verbose - verbose output\n");
}

void verbose_output(const struct inotify_event *event) {
    // verbose output

    if( !(event->mask & FLAGS) ) {
        printf("event captured with no relevant action\n");
        return;
    }
    printf("events detected:\n");
    if(event->mask & IN_DELETE) {
         printf("IN_DELETE (+) -> '%s'", event->name);
    }
    if(event->mask & IN_DELETE_SELF) {
        printf("IN_DELETE_SELF (watcher is implicitly removed)");
    }
    if(event->mask & IN_MOVE_SELF) {
        printf("IN_MOVE_SELF");
    }
    if(event->mask & IN_MOVE) {
        printf("IN_MOVE '%s'", event->name);
    }
    if(event->mask & IN_CLOSE_WRITE) {
        printf("IN_CLOSE_WRITE (+) -> '%s'", event->name);
    }
    if(event->mask & IN_CREATE) {
        printf("IN_CREATE (+) -> '%s'", event->name);
    }
    if(event->mask & IN_MODIFY) {
        printf("IN_MODIFY (+) -> '%s'", event->name);
    }
    if(event->mask & IN_ATTRIB) {
        printf("IN_ATTRIB (*) -> '%s'", event->name);
    }

    if (event->mask & IN_ISDIR) {
        printf(" [directory]\n");
    } else {
        printf(" [file]\n");
    }
}

uint8_t strequal(char* a, char* b, unsigned int len) {
    for(unsigned int i = 0; i < len; i++) if(a[i] != b[i]) return 0;
    return 1;
}

uint32_t strlength(char* a) {
    uint32_t i;
    for(i = 0; a[i] != 0; i++);
    return i;
}

int is_ignored(OnsaveConfig* config, const struct inotify_event* event, char** argv) {
    unsigned int filename_len = MIN(event->len, strlength((char*)event->name));
    if(!filename_len) return 0;
    for(uint32_t i = 0; i < config->ignore_len; i++) {
        char* ignored = argv[config->ignore_arr[i]];
        unsigned int min = MIN(filename_len, strlength(ignored));
        if(strequal(ignored, (char*)event->name, min)) return 1;
    }
    return 0;
}

#define SET_BIT_AND_CONTINUE(var, bit) var |= bit; continue;

void parse_args(unsigned int argc, char** argv, OnsaveConfig* config) {

    // nullify struct
    for(unsigned int i = 0; i < sizeof(OnsaveConfig); i++) ((uint8_t*)config)[i] = 0x00;
    // go through every command line argument
    uint8_t next_is_parameter = 0;
    for(unsigned int arg_counter = 1; arg_counter < argc && !(config->flags & ERROR_FLAG); arg_counter++) {
        char* arg = argv[arg_counter];
        // not argument
        if(arg[0] != '-') {
            // arg list end
            if(!next_is_parameter) {
                if(arg_counter == argc - 1) {
                    SET_BIT_AND_CONTINUE(config->flags, ERROR_FLAG);
                }
                config->file_idx = arg_counter;
                return;
            // parameter
            } else {
                switch (next_is_parameter) {
                    case IGNORE_FLAG: {
                        config->ignore_arr = realloc(config->ignore_arr, ++config->ignore_len);
                        config->ignore_arr[config->ignore_len - 1] = arg_counter;
                        break;
                    }
                    default: {
                        config->flags |= ERROR_FLAG;
                    }
                }
                next_is_parameter = 0;
            }
        } else {
            uint8_t long_name_offset = arg[1] == '-';
            switch(arg[1 + long_name_offset]) {
                case 'h': {
                    SET_BIT_AND_CONTINUE(config->flags, HELP_FLAG);
                }
                case 'o': {
                    SET_BIT_AND_CONTINUE(config->flags, ONCE_FLAG);
                }
                case 'g': {
                    SET_BIT_AND_CONTINUE(config->flags, GIT_FLAG);
                }
                case 'i': {
                    SET_BIT_AND_CONTINUE(next_is_parameter, IGNORE_FLAG);
                }
                case 'v': {
                    SET_BIT_AND_CONTINUE(config->flags, VERBOSE_FLAG);
                }
                case 'q': {
                    SET_BIT_AND_CONTINUE(config->flags, QUIET_FLAG);
                }
                default: {
                    SET_BIT_AND_CONTINUE(config->flags, ERROR_FLAG);
                }
            }
        }
    }
}

int main(int argc, char** argv) {

    // check if there is enough arguments (program, file/directory and command at least)
    if(argc < 3) {
        help();
        exit(EXIT_FAILURE);
    }
    // get arguments
    OnsaveConfig config;
    parse_args(argc, argv, &config);

    // throw parsing error
    if(config.flags & ERROR_FLAG || config.flags & HELP_FLAG) {
        if(!(config.flags & QUIET_FLAG)) help();
        exit(config.flags & ERROR_FLAG ? EXIT_FAILURE : EXIT_SUCCESS);
    }

    char buf[4096] = {0};
    char* filename = argv[config.file_idx];

    // initialize inotify_init
    int fd = inotify_init();
    if( fd == -1 ) {
        perror("inotify_init");
        exit(EXIT_FAILURE);
    }

    // watch file
    int wd = inotify_add_watch(fd, filename, WATCH_FLAGS);
    struct inotify_event *event;

    do {
        ssize_t len = read(fd, buf, sizeof(buf));
        event = (struct inotify_event*) buf;

        if(event->mask & FLAGS) {
            if(!is_ignored(&config, event, argv)) { 
                system(argv[config.file_idx+1]);

                if( (config.flags & VERBOSE_FLAG) && !(config.flags & QUIET_FLAG) ) verbose_output(event);
                continue;
            }
        }

        if(event->mask & IN_UNMOUNT) {
            if(!(config.flags & QUIET_FLAG)) fprintf(stderr, "ERROR: filesystem was unmounted\n");
            exit(EXIT_FAILURE);
        }

        if(event->mask & IN_IGNORED) {

            close(fd);
            // initialize inotify_init
            fd = inotify_init();
            if( fd == -1 ) {
                perror("inotify_init");
                exit(EXIT_FAILURE);
            }

            // watch file
            wd = inotify_add_watch(fd, filename, WATCH_FLAGS);
        }
    } while(!(config.flags & ONCE_FLAG));

    free(config.ignore_arr);

    return 0;
}
