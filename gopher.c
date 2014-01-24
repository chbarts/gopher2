#define _POSIX_SOURCE
#include <linux/limits.h>
#include <sys/sendfile.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include "queue.h"

static q *conns = NULL, *threads = NULL;

static pthread_mutex_t mutex, cmutex;

static pthread_cond_t notempty = PTHREAD_COND_INITIALIZER;

static int sfd;

volatile int ctr;

static void do_gopher(int fd)
{
    char buf[PATH_MAX], err[PATH_MAX * 2], *end;
    struct stat sbuf;
    ssize_t len;
    off_t offset = 0;
    int ifd;

    if ((len = read(fd, buf, PATH_MAX)) == -1) {
        len =
            snprintf(err, PATH_MAX * 2,
                     "3Error reading input: %s\tfake\terror.host\t1\r\n.\r\n",
                     strerror(errno));
        write(fd, err, len);
        return;
    }


    if ((buf[0] == '\n') || (buf[0] == '\r') || (buf[0] == '\t')) {
        /* Received request for selector list. */
        if ((ifd = open(".selectors", O_RDONLY)) == -1) {
#define ERR1 "3No .selectors file. Bug administrator to fix.\tfake\terror.host\t1\r\n.\r\n"
            write(fd, ERR1, sizeof(ERR1) - 1);
            return;
        }

    } else {
        if ((end = memchr(buf, '\r', PATH_MAX))
            || (end = memchr(buf, '\n', PATH_MAX))) {
            *end = '\0';
        } else {
#define ERR2 "3Malfomed request\tfake\terror.host\t1\r\n.\r\n"
            write(fd, ERR2, sizeof(ERR2) - 1);
            return;
        }

        if ((ifd = open(buf, O_RDONLY)) == -1) {
            len =
                snprintf(err, PATH_MAX * 2,
                         "3'%s' does not exist (no handler found)\tfake\terror.host\t1\r\n.\r\n",
                         buf);
            write(fd, err, len);
            return;
        }
    }

    if (fstat(ifd, &sbuf) == -1) {
#define ERR3 "3fstat failed\tfake\terror.host\t1\r\n.\r\n"
        write(fd, ERR3, sizeof(ERR3) - 1);
        close(ifd);
        return;
    }

    if (!S_ISREG(sbuf.st_mode) && !S_ISFIFO(sbuf.st_mode)
        && !S_ISLNK(sbuf.st_mode)) {
#define ERR4 "3invalid file type\tfake\terror.host\t1\r\n.\r\n"
        write(fd, ERR4, sizeof(ERR4) - 1);
        close(ifd);
        return;
    }

    if ((len = sendfile(fd, ifd, &offset, sbuf.st_size)) < sbuf.st_size) {
        if (len == -1) {
#define ERR5 "3sendfile() failed\tfake\terror.host\t1\r\n.\r\n"
            write(fd, ERR5, sizeof(ERR5) - 1);
            close(ifd);
            return;
        } else {
            while ((len =
                    sendfile(fd, ifd, &offset,
                             sbuf.st_size - offset)) <
                   (sbuf.st_size - offset)) {
                if (len == -1)
                    goto abend;
            }
        }
    }

  abend:
    close(ifd);
    return;
}

static void *threadFunc(void *param)
{
    int cfd = 0, *iptr;

#ifdef DEBUG
    fprintf(stderr, "I'm here!\n");
#endif

    pthread_mutex_lock(&cmutex);
    ctr++;
    pthread_mutex_unlock(&cmutex);

#ifdef DEBUG
    fprintf(stderr, "ctr incremented\n");
#endif

    while (true) {
        pthread_mutex_lock(&mutex);
        while (q_len(conns) == 0) {
            pthread_cond_wait(&notempty, &mutex);
        }

        iptr = q_pop(conns);
        pthread_mutex_unlock(&mutex);

        cfd = *iptr;
        free(iptr);

        if (cfd == -1)
            break;

        pthread_mutex_lock(&cmutex);
        ctr--;
        pthread_mutex_unlock(&cmutex);

        do_gopher(cfd);

        shutdown(cfd, SHUT_RDWR);
        close(cfd);

        pthread_mutex_lock(&cmutex);
        ctr++;
        pthread_mutex_unlock(&cmutex);
    }

    pthread_mutex_lock(&cmutex);
    ctr--;
    pthread_mutex_unlock(&cmutex);
    pthread_exit(NULL);
}

static void handler(int sig)
{
    int ecode = EXIT_SUCCESS, *pint;
    pthread_t *ppth;
    size_t i;
    void *ret;

    for (i = 0; i < q_len(threads); i++) {
        pint = malloc(sizeof(int));
        *pint = -1;
        pthread_mutex_lock(&mutex);
        q_add(conns, pint);
        pthread_cond_broadcast(&notempty);
        pthread_mutex_unlock(&mutex);
    }

    while (q_len(threads) > 0) {
        ppth = q_pop(threads);
        if (pthread_join(*ppth, &ret) != 0) {
            perror("pthread_join failed");
            ecode = EXIT_FAILURE;
        }

        free(ppth);
    }

    q_free(threads);
    q_free(conns);
    close(sfd);
    pthread_mutex_destroy(&mutex);
    pthread_mutex_destroy(&cmutex);
    exit(ecode);
}

static int sig_handle(int sig, void (*hndlr) (int))
{
    struct sigaction act;

    act.sa_handler = hndlr;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    return sigaction(sig, &act, NULL);
}

int main(int argc, char *argv[])
{
    int cfd, port = 70, nthreads = 4;
    struct sockaddr_in sa;
    void *ret;

    if (sig_handle(SIGPIPE, SIG_IGN) == -1) {
        perror("sigaction() SIGPIPE");
        exit(EXIT_FAILURE);
    }

    if (sig_handle(SIGINT, handler) == -1) {
        perror("sigaction() SIGINT");
        exit(EXIT_FAILURE);
    }

    if (sig_handle(SIGHUP, handler) == -1) {
        perror("sigaction() SIGHUP");
        exit(EXIT_FAILURE);
    }

    if (sig_handle(SIGTERM, handler) == -1) {
        perror("sigaction() SIGTERM");
        exit(EXIT_FAILURE);
    }

    if (sig_handle(SIGUSR1, handler) == -1) {
        perror("sigaction() SIGUSR1");
        exit(EXIT_FAILURE);
    }

    if (sig_handle(SIGUSR2, handler) == -1) {
        perror("sigaction() SIGUSR2");
        exit(EXIT_FAILURE);
    }

    if (sig_handle(SIGURG, handler) == -1) {
        perror("sigaction() SIGURG");
        exit(EXIT_FAILURE);
    }

    if (argc == 3) {
        nthreads = atoi(argv[1]);
        port = atoi(argv[2]);
    } else if (argc == 2) {
        nthreads = atoi(argv[1]);
    } else if (argc > 3) {
        fprintf(stderr, "usage: server [nthreads [port]]\n");
        exit(EXIT_FAILURE);
    }

    if ((sfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    memset(&sa, 0, sizeof(sa));
    sa.sin_family = PF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sfd, (const struct sockaddr *) &sa, sizeof(sa)) == -1) {
        perror("bind() failed");
        close(sfd);
        exit(EXIT_FAILURE);
    }

    if (listen(sfd, 50) == -1) {
        perror("listen() failed");
        close(sfd);
        exit(EXIT_FAILURE);
    }

    if (pthread_mutex_init(&mutex, NULL) != 0) {
        perror("pthread_mutex init failed");
        close(sfd);
        exit(EXIT_FAILURE);
    }

    if (pthread_mutex_init(&cmutex, NULL) != 0) {
        perror("pthread_mutex init (cmutex) failed");
        pthread_mutex_destroy(&mutex);
        close(sfd);
        exit(EXIT_FAILURE);
    }

    if ((threads = q_new()) == NULL) {
        fprintf(stderr, "q_new() failed\n");
        handler(0);
    }

    if ((conns = q_new()) == NULL) {
        fprintf(stderr, "q_new() failed\n");
        handler(0);
    }

    ctr = 0;

    for (int i = 0; i < nthreads; i++) {
        pthread_t *thread;
        thread = malloc(sizeof(pthread_t));

        if (pthread_create(thread, NULL, threadFunc, NULL) != 0) {
            perror("pthread_create failed");
            handler(0);
        }

        q_add(threads, thread);
    }

#ifdef DEBUG
    fprintf(stderr, "ctr = %d\n", ctr);
#endif

    while (true) {
        int *n;
        if ((cfd = accept(sfd, NULL, NULL)) == -1) {
            perror("accept() failed");
            handler(0);
        }

        n = malloc(sizeof(int));
        *n = cfd;

        pthread_mutex_lock(&cmutex);
        if (ctr == 0) {
            pthread_mutex_unlock(&cmutex);
            for (int i = 0; i < 2; i++) {
                pthread_t *thread;
                thread = malloc(sizeof(pthread_t));
                if (pthread_create(thread, NULL, threadFunc, NULL) != 0) {
                    perror("pthread_create() failed");
                    break;
                }

                q_add(threads, thread);
            }

        } else {
            pthread_mutex_unlock(&cmutex);
        }

        pthread_mutex_lock(&mutex);
        q_add(conns, n);
        pthread_cond_broadcast(&notempty);
        pthread_mutex_unlock(&mutex);
    }

    return 0;
}
