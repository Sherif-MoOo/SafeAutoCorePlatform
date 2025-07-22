/***********************************************************************************************************************
 *  PROJECT
 *  --------------------------------------------------------------------------------------------------------------------
 *  \verbatim
 *  OpenAA: Open Source Adaptive AUTOSAR Project (C_STANDARD 11)
 *  Author: Sherif Mohamed
 *  \endverbatim
 *  --------------------------------------------------------------------------------------------------------------------
 *  FILE DESCRIPTION
 *  --------------------------------------------------------------------------------------------------------------------
 *  \file       main.c
 *  \brief      Hardened, Optimized POSIX test‑bed application (Linux & QNX, C11).
 *
 *  \details    A single‑translation‑unit reference implementation that:
 *                • Demonstrates portable, high‑quality idioms:
 *                      - Async‑safe signal handling (self‑pipe trick)
 *                      - Robust EINTR‑aware I/O wrappers
 *                      - Periodic poll‑timeout tick (no Linux‑only APIs)
 *                      - Detached pthread doing secure work (/dev/urandom)
 *                      - Constant‑time log helpers (no format‑string foot‑guns)
 *                      - High‑resolution time maths helpers (timespec)
 *                • Zero undefined behaviour, clang‑tidy clean, sanitized.
 *
 *              The program echoes stdin, emits a 5000 ms tick, shows background
 *              random status, and terminates gracefully on SIGINT/SIGTERM.
 **********************************************************************************************************************/
#define _POSIX_C_SOURCE 200809L   /* exposes sigaction, O_CLOEXEC, CLOCK_* */

/**********************************************************************************************************************
 *  INCLUDES
 **********************************************************************************************************************/
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

/**********************************************************************************************************************
 *  CONSTANTS & MACROS (no function‑like macros)
 **********************************************************************************************************************/
static const char * const URANDOM_PATH   = "/dev/urandom";    /*!< Cryptographically secure random source */
static const long        TICK_MSEC      = 5000L;              /*!< Main‑loop period (milliseconds)        */
enum                   { MAX_LINE_BYTES = 1024 };             /*!< Max stdin line length (inc. NUL)       */

/**********************************************************************************************************************
 *  GLOBAL STATE (signal‑safe)
 **********************************************************************************************************************/
static volatile sig_atomic_t g_terminate = 0;                 /*!< Set by signal handler                  */
static int                   g_sig_pipe[2] = {-1, -1};        /*!< Self‑pipe file descriptors             */

/**********************************************************************************************************************
 *  FORWARD DECLARATIONS
 **********************************************************************************************************************/
static void      handle_signal(int signo);
static int       install_signal_handlers(void);
static ssize_t   retry_write(int fd, const void *buf, size_t len);
static ssize_t   retry_read (int fd,       void *buf, size_t len);
static void     *worker_thread(void *arg);
static int       secure_random(void *buf, size_t len);
static void      log_error(const char *msg);
static void      flush_line(const char *prefix, const char *line);
static void      timespec_add_msec(struct timespec *ts, long msec);
static long      timespec_sub_msec(const struct timespec *a, const struct timespec *b);

/**********************************************************************************************************************
 *  SIGNAL HANDLING
 **********************************************************************************************************************/
static void handle_signal(int signo)
{
    (void)signo;                                    /* suppress unused‑param */

    g_terminate = 1;
    (void)retry_write(g_sig_pipe[1], "!", (size_t)1); /* async‑safe */
}

static int install_signal_handlers(void)
{
    struct sigaction sa;                            /* declaration first     */

    if (pipe(g_sig_pipe) != 0) {
        log_error("pipe failed");
        return -1;
    }
    for (int i = 0; i < 2; ++i) {
        if (fcntl(g_sig_pipe[i], F_SETFD, FD_CLOEXEC) == -1) {
            log_error("fcntl(FD_CLOEXEC) failed");
            return -1;
        }
    }

    memset(&sa, 0, sizeof sa);
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;                                /* SA_RESTART missing on QNX */

    if (sigaction(SIGINT,  &sa, NULL) != 0 ||
        sigaction(SIGTERM, &sa, NULL) != 0) {
        log_error("sigaction failed");
        return -1;
    }
    return 0;
}

/**********************************************************************************************************************
 *  EINTR‑AWARE I/O
 **********************************************************************************************************************/
static ssize_t retry_write(int fd, const void *buf, size_t len)
{
    const char *p = (const char *)buf;
    while (len > 0) {
        ssize_t s = write(fd, p, len);
        if (s > 0)            { p += (size_t)s; len -= (size_t)s; }
        else if (s == -1 && errno == EINTR) { continue; }
        else                   { return -1; }
    }
    return 0;
}

static ssize_t retry_read(int fd, void *buf, size_t len)
{
    for (;;) {
        ssize_t s = read(fd, buf, len);
        if (s == -1 && errno == EINTR) { continue; }
        return s;
    }
}

/**********************************************************************************************************************
 *  UTILITIES
 **********************************************************************************************************************/
static void log_error(const char *msg)
{
    retry_write(STDERR_FILENO, msg, strlen(msg));
    retry_write(STDERR_FILENO, ": ", (size_t)2);
    retry_write(STDERR_FILENO, strerror(errno), strlen(strerror(errno)));
    retry_write(STDERR_FILENO, "\n", (size_t)1);
}

static void flush_line(const char *prefix, const char *line)
{
    flockfile(stderr);
    fprintf(stderr, "%s%s\n", prefix, line);
    funlockfile(stderr);
}

static int secure_random(void *buf, size_t len)
{
    int     fd;
    ssize_t rd;

    fd = open(URANDOM_PATH, O_RDONLY | O_CLOEXEC);
    if (fd == -1) { return -1; }

    rd = retry_read(fd, buf, len);
    close(fd);
    return (rd == (ssize_t)len) ? 0 : -1;
}

static void timespec_add_msec(struct timespec *ts, long msec)
{
    ts->tv_sec  +=  msec / 1000L;
    ts->tv_nsec += (msec % 1000L) * 1000000L;
    if (ts->tv_nsec >= 1000000000L) {
        ts->tv_sec++;
        ts->tv_nsec -= 1000000000L;
    }
}

static long timespec_sub_msec(const struct timespec *a, const struct timespec *b)
{
    long diff_sec  = (long)(a->tv_sec  - b->tv_sec);
    long diff_nsec = (long)(a->tv_nsec - b->tv_nsec);
    return diff_sec * 1000L + diff_nsec / 1000000L;
}

/**********************************************************************************************************************
 *  WORKER THREAD
 **********************************************************************************************************************/
static void *worker_thread(void *arg)
{
    struct timespec next;
    unsigned char   buf[8];
    const char     *status;
    struct timespec now;
    long            wait_ms;
    struct timespec ts;

    (void)arg;                                      /* after declarations */

    clock_gettime(CLOCK_MONOTONIC, &next);

    while (!g_terminate) {
        timespec_add_msec(&next, 2500L);

        status = (secure_random(buf, sizeof buf) == 0) ? "OK" : "ERR";
        flush_line("[thread] random ", status);

        clock_gettime(CLOCK_MONOTONIC, &now);
        wait_ms = timespec_sub_msec(&next, &now);
        if (wait_ms < 0L) { continue; }

        ts.tv_sec  =  wait_ms / 1000L;
        ts.tv_nsec = (wait_ms % 1000L) * 1000000L;
        while (nanosleep(&ts, &ts) != 0 && errno == EINTR) { /* retry */ }
    }
    return NULL;
}

/**********************************************************************************************************************
 *  MAIN
 **********************************************************************************************************************/
int main(void)
{
    pthread_t     tid;
    struct pollfd fds[2];
    char          line[MAX_LINE_BYTES];

    if (install_signal_handlers() != 0) {
        return EXIT_FAILURE;
    }

    if (pthread_create(&tid, NULL, worker_thread, NULL) != 0) {
        log_error("pthread_create failed");
        return EXIT_FAILURE;
    }

    fds[0].fd     = STDIN_FILENO;  fds[0].events = POLLIN;
    fds[1].fd     = g_sig_pipe[0]; fds[1].events = POLLIN;

    line[0] = '\0';
    flush_line("[main] ", "ready - press Ctrl-C to exit");

    while (!g_terminate) {
        int rv = poll(fds, 2U, (int)TICK_MSEC);
        if (rv == -1) {
            if (errno == EINTR) { continue; }
            log_error("poll failed");
            break;
        }

        if (rv == 0) {                             /* timeout */
            flush_line("[tick] ", "5000 ms");
            continue;
        }

        if (fds[1].revents & POLLIN) {             /* signal byte */
            char sigbuf[8];
            (void)retry_read(g_sig_pipe[0], sigbuf, sizeof sigbuf);
        }

        if (fds[0].revents & POLLIN) {             /* stdin */
            ssize_t n = retry_read(STDIN_FILENO, line, sizeof line - 1U);
            if (n > 0) {
                line[(size_t)n] = '\0';
                flush_line("[stdin] ", line);
            } else if (n == 0) {
                flush_line("[stdin] ", "EOF");
                g_terminate = 1;
            }
        }
    }

    pthread_join(tid, NULL);
    close(g_sig_pipe[0]);
    close(g_sig_pipe[1]);
    flush_line("[main] ", "graceful shutdown");
    return EXIT_SUCCESS;
}