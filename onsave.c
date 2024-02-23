#include <stdlib.h>
#include <sys/inotify.h>
#include <stdio.h>
#include <libnotify/notification.h>
#include <libnotify/notify.h>

#define FLAGS IN_DELETE | IN_DELETE_SELF | IN_MOVE_SELF | IN_MOVE | IN_CREATE | IN_MODIFY | IN_ATTRIB

int main(int argc, char** argv) {

    int fd = inotify_init();
    if( fd == -1 ) {
        perror("inotify_init");
        exit(EXIT_FAILURE);
    }

    char* filename = "README.md";
    int wd = inotify_add_watch(fd, filename, FLAGS);


    char buf[4096] = {0};
    ssize_t len = read(fd, buf, sizeof(buf));
    const struct inotify_event *event;
    for (char *ptr = buf; ptr < buf + len; ptr += sizeof(struct inotify_event) + event->len) {
        event = (struct inotify_event*) ptr;

           /* Print event type. */
           if (event->mask & IN_OPEN)
               printf("IN_OPEN: ");
           if (event->mask & IN_CLOSE_NOWRITE)
               printf("IN_CLOSE_NOWRITE: ");
           if (event->mask & IN_CLOSE_WRITE)
               printf("IN_CLOSE_WRITE: ");
           if (event->len)
               printf("%s", event->name);
           if (event->mask & IN_ISDIR)
               printf(" [directory]\n");
           else
               printf(" [file]\n");
    }

    return 0;
}
