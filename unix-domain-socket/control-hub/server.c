#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

const char      *socket_path = "/tmp/control-hub.socket";

#define dbgprint(fmt, args...) \
    do { \
        fprintf(stderr, fmt, ##args); \
    } while (0)

struct control_handle {
    int             socket;
};

void           *
handle_port(void *data)
{
    int             rc;
    char            buf[4];
    struct control_handle *handle = data;
    int             cl = handle->socket;

    dbgprint("thread %d started, with socket %d\n", (int)pthread_self(), cl);

    while ((rc = read(cl, buf, sizeof(buf))) > 0) {
        dbgprint("received %x %x %x %x\n", buf[0], buf[1], buf[2], buf[3]);
        //TODO: dispatch based on the command received from client
        
    }

    if (rc == -1) {
        perror("read");
        goto exit;
    } else if (rc == 0) {
        printf("EOF\n");
        goto exit;
    }
exit:
    dbgprint("thread %d going down, clean up\n", (int)pthread_self());
    close(handle->socket);
    free(handle);
    return 0;
}

int 
main(int argc, char *argv[])
{
    struct sockaddr_un addr;
    int             fd, cl;
    int             ret;
    struct control_handle *ch;
    pthread_t thid;

    if (argc > 1)
        socket_path = argv[1];

    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket error");
        exit(-1);
    }
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    if (*socket_path == '\0') {
        *addr.sun_path = '\0';
        strncpy(addr.sun_path + 1, socket_path + 1, sizeof(addr.sun_path) - 2);
    } else {
        strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);
        unlink(socket_path);
    }

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("bind error");
        exit(-1);
    }
    if (listen(fd, 5) == -1) {
        perror("listen error");
        exit(-1);
    }
    while (1) {
        if ((cl = accept(fd, NULL, NULL)) == -1) {
            perror("accept error");
            continue;
        }
        ch = (struct control_handle *)malloc(sizeof(struct control_handle));
        if (!ch) {
            dbgprint("malloc failed, abort connection\n");
            close(cl);
            continue;
        }
        ch->socket = cl;
        dbgprint("creating thread with socket %d\n", ch->socket);
        ret = pthread_create(&thid, NULL, handle_port, ch);
        if (ret) {
            dbgprint("cannot create thread: %s\n", strerror(ret));
            free(ch);
            close(cl);
        }
        pthread_detach(thid);

    }


    return 0;
}
