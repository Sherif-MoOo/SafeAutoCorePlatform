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
 *  \brief      Hardened, Optimized POSIX test-bed application (Linux & QNX).
 *
 *  \details    single-translation-unit reference implementation that demonstrates:
 *                • Multi-threading (4 worker threads with different roles)
 *                • Inter-thread communication via pipes and shared memory
 *                • File system operations and monitoring
 *                • Process resource monitoring
 *                • logging with timestamps and thread IDs
 *                • Robust signal handling for multiple signals
 *                • Memory-mapped file operations
 *                • POSIX semaphores and mutexes
 *                • High-resolution timers and performance measurements
 **********************************************************************************************************************/
#define _POSIX_C_SOURCE 200809L

/**********************************************************************************************************************
 *  INCLUDES
 **********************************************************************************************************************/
#include <dirent.h>
#include <stdbool.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <poll.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

/**********************************************************************************************************************
 *  CONSTANTS & MACROS
 **********************************************************************************************************************/
static const char * const URANDOM_PATH     = "/dev/urandom";
static const char * const WORK_DIR         = "./posix_test_work";
static const char * const MMAP_FILE        = "./posix_test_work/mmap_data.bin";
static const char * const LOG_FILE         = "./posix_test_work/test.log";
static const char * const FIFO_PATH        = "./posix_test_work/test.fifo";
static const long         TICK_MSEC        = 5000L;

enum { 
    MAX_LINE_BYTES   = 1024,
    MAX_LOG_MSG      = 2048,
    MAX_LOG_FINAL    = 4096,  /* Final formatted log message buffer */
    MAX_THREADS      = 4,
    MAX_PATH         = 4096,
    PIPE_BUF_SIZE    = 512,
    SHARED_MEM_SIZE  = 1024,
    MMAP_SIZE        = 4096,
    HOST_NAME_SIZE   = 256   /* Fixed size for hostname buffer */
};

/**********************************************************************************************************************
 *  TYPE DEFINITIONS
 **********************************************************************************************************************/
typedef struct {
    pthread_mutex_t mutex;
    sem_t           sem_prod;
    sem_t           sem_cons;
    atomic_long     counter;
    char            buffer[SHARED_MEM_SIZE];
    size_t          write_pos;
    size_t          read_pos;
} shared_data_t;

typedef struct {
    int             thread_id;
    pthread_t       tid;
    char            name[32];
    atomic_int      active;
    int             pipe_fd[2];
} thread_info_t;

typedef enum {
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_DEBUG
} log_level_t;

/**********************************************************************************************************************
 *  GLOBAL STATE
 **********************************************************************************************************************/
static volatile sig_atomic_t g_terminate    = 0;
static volatile sig_atomic_t g_reload_conf  = 0;
static int                   g_sig_pipe[2]  = {-1, -1};
static int                   g_log_fd       = -1;
static pthread_mutex_t       g_log_mutex    = PTHREAD_MUTEX_INITIALIZER;
static shared_data_t        *g_shared_data  = NULL;
static thread_info_t         g_threads[MAX_THREADS];
static void                 *g_mmap_addr    = MAP_FAILED;
static size_t                g_mmap_size    = 0;
static int                   g_fifo_fd      = -1;

/**********************************************************************************************************************
 *  FORWARD DECLARATIONS
 **********************************************************************************************************************/
/* Signal handling */
static void      handle_signal(int signo);
static int       install_signal_handlers(void);

/* I/O operations */
static ssize_t   retry_write(int fd, const void *buf, size_t len);
static ssize_t   retry_read(int fd, void *buf, size_t len);
static int       set_nonblocking(int fd);

/* Logging */
static void      log_message(log_level_t level, const char *thread_name, const char *format, ...);
static int       init_logging(void);
static void      close_logging(void);

/* Thread functions */
static void     *random_worker_thread(void *arg);
static void     *file_monitor_thread(void *arg);
static void     *process_monitor_thread(void *arg);
static void     *system_info_thread(void *arg);

/* Utilities */
static int       secure_random(void *buf, size_t len);
static void      timespec_add_msec(struct timespec *ts, long msec);
static long      timespec_sub_msec(const struct timespec *a, const struct timespec *b);
static int       create_work_directory(void);
static int       init_shared_memory(void);
static void      cleanup_shared_memory(void);
static int       init_memory_mapped_file(void);
static void      cleanup_memory_mapped_file(void);
static int       init_fifo(void);
static void      cleanup_fifo(void);
static void      get_timestamp(char *buf, size_t len);
static void      print_system_info(void);
static int       monitor_directory(const char *path);
static void      demonstrate_fork_exec(void);

/**********************************************************************************************************************
 *  SIGNAL HANDLING
 **********************************************************************************************************************/
static void handle_signal(int signo)
{
    switch (signo) {
        case SIGINT:
        case SIGTERM:
            g_terminate = 1;
            break;
        case SIGHUP:
            g_reload_conf = 1;
            break;
        default:
            break;
    }
    (void)retry_write(g_sig_pipe[1], "!", (size_t)1);
}

static int install_signal_handlers(void)
{
    struct sigaction sa;
    sigset_t block_mask;

    if (pipe(g_sig_pipe) != 0) {
        perror("pipe");
        return -1;
    }

    for (int i = 0; i < 2; ++i) {
        if (fcntl(g_sig_pipe[i], F_SETFD, FD_CLOEXEC) == -1 ||
            set_nonblocking(g_sig_pipe[i]) == -1) {
            perror("fcntl");
            return -1;
        }
    }

    /* Block all signals during handler execution */
    sigfillset(&block_mask);

    memset(&sa, 0, sizeof sa);
    sa.sa_handler = handle_signal;
    sa.sa_mask = block_mask;
    sa.sa_flags = 0;

    if (sigaction(SIGINT,  &sa, NULL) != 0 ||
        sigaction(SIGTERM, &sa, NULL) != 0 ||
        sigaction(SIGHUP,  &sa, NULL) != 0) {
        perror("sigaction");
        return -1;
    }

    /* Ignore SIGPIPE for FIFO operations */
    sa.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &sa, NULL) != 0) {
        perror("sigaction SIGPIPE");
        return -1;
    }

    return 0;
}

/**********************************************************************************************************************
 *  I/O OPERATIONS
 **********************************************************************************************************************/
static ssize_t retry_write(int fd, const void *buf, size_t len)
{
    const char *p = (const char *)buf;
    size_t remaining = len;
    
    while (remaining > 0) {
        ssize_t written = write(fd, p, remaining);
        if (written > 0) {
            p += (size_t)written;
            remaining -= (size_t)written;
        } else if (written == -1 && errno == EINTR) {
            continue;
        } else {
            return -1;
        }
    }
    return (ssize_t)len;
}

static ssize_t retry_read(int fd, void *buf, size_t len)
{
    for (;;) {
        ssize_t n = read(fd, buf, len);
        if (n == -1 && errno == EINTR) {
            continue;
        }
        return n;
    }
}

static int set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

/**********************************************************************************************************************
 *  LOGGING
 **********************************************************************************************************************/
static void get_timestamp(char *buf, size_t len)
{
    struct timespec ts;
    struct tm tm_info;
    int year, mon, day, hour, min, sec;
    long msec;
    
    clock_gettime(CLOCK_REALTIME, &ts);
    localtime_r(&ts.tv_sec, &tm_info);
    
    /* Ensure values are within expected ranges to satisfy compiler */
    year = tm_info.tm_year + 1900;
    mon  = tm_info.tm_mon + 1;
    day  = tm_info.tm_mday;
    hour = tm_info.tm_hour;
    min  = tm_info.tm_min;
    sec  = tm_info.tm_sec;
    msec = ts.tv_nsec / 1000000L;
    
    /* Clamp values to ensure they fit in format specifiers */
    if (year < 0) year = 0;
    if (year > 9999) year = 9999;
    if (mon < 1) mon = 1;
    if (mon > 12) mon = 12;
    if (day < 1) day = 1;
    if (day > 31) day = 31;
    if (hour < 0) hour = 0;
    if (hour > 23) hour = 23;
    if (min < 0) min = 0;
    if (min > 59) min = 59;
    if (sec < 0) sec = 0;
    if (sec > 59) sec = 59;
    if (msec < 0) msec = 0;
    if (msec > 999) msec = 999;
    
    snprintf(buf, len, "%04d-%02d-%02d %02d:%02d:%02d.%03ld",
             year, mon, day, hour, min, sec, msec);
}

/**********************************************************************************************************************
 *  LOGGING FUNCTION
 **********************************************************************************************************************/
static int vstr_printf(char *dst, size_t dst_sz,
                       const char *fmt, va_list ap)
{
    /* By calling through a plain function pointer we strip the
       built‑in printf attribute; the compiler stops complaining.   */
    int (*fn)(char *, size_t, const char *, va_list) = vsnprintf;
    return fn(dst, dst_sz, fmt, ap);
}

/* ───────────────────────── log_message ─────────────────────── */
static void log_message(log_level_t level,
                        const char *thread_name,
                        const char *fmt, ...)
{
    /* constants */
    static const char *const LVL_TXT[] = { "INFO", "WARN", "ERROR", "DEBUG" };
    enum { TS_SZ = 32, BODY_SZ = MAX_LOG_MSG, HDR_SZ = 128,
           LVL_W = 5,  THR_W  = 12 };

    /* all C90‑style declarations first */
    char   ts[TS_SZ];
    char   body[BODY_SZ];
    size_t body_len;

    const char *lvl_txt;
    size_t lvl_len, lvl_pad_cnt;
    char   lvl_pad[LVL_W + 2];

    size_t thr_len_full, thr_len_print, thr_pad_cnt;
    char   thr_buf[THR_W + 1];
    char   thr_pad[THR_W + 2];

    char   hdr[HDR_SZ];
    int    hdr_len;
    int    out_fd;

    va_list ap;

    /* ---------------------------------------------------------------- */
    if (level < LOG_INFO || level > LOG_DEBUG) level = LOG_INFO;
    if (!thread_name || thread_name[0] == '\0') thread_name = "unknown";

    get_timestamp(ts, sizeof ts);

    va_start(ap, fmt);
    body_len = (size_t)vstr_printf(body, sizeof body, fmt, ap);
    va_end(ap);

    if (body_len == (size_t)-1 || body_len >= sizeof body) {
        static const char err_msg[] = "log formatting error";
        size_t copy_len = sizeof err_msg - 1;
        if (copy_len >= sizeof body) copy_len = sizeof body - 1;
        memcpy(body, err_msg, copy_len);
        body[copy_len] = '\0';
    }

    lvl_txt     = LVL_TXT[level];
    lvl_len     = strlen(lvl_txt);
    lvl_pad_cnt = (lvl_len < LVL_W) ? (LVL_W - lvl_len) : 0;
    memset(lvl_pad, ' ', lvl_pad_cnt + 1);
    lvl_pad[lvl_pad_cnt + 1] = '\0';

    thr_len_full  = strlen(thread_name);
    thr_len_print = (thr_len_full > THR_W) ? THR_W : thr_len_full;
    memcpy(thr_buf, thread_name, thr_len_print);
    thr_buf[thr_len_print] = '\0';
    thr_pad_cnt = (thr_len_print < THR_W) ? (THR_W - thr_len_print) : 0;
    memset(thr_pad, ' ', thr_pad_cnt + 1);
    thr_pad[thr_pad_cnt + 1] = '\0';

    hdr_len = snprintf(hdr, sizeof hdr,
                       "[%s] [%s]%s [%s]%s ",
                       ts, lvl_txt, lvl_pad, thr_buf, thr_pad);
    if (hdr_len < 0 || (size_t)hdr_len >= sizeof hdr) return;

    out_fd = (level == LOG_WARN || level == LOG_ERROR) ? STDERR_FILENO
                                                       : STDOUT_FILENO;

    pthread_mutex_lock(&g_log_mutex);
    retry_write(out_fd, hdr,  (size_t)hdr_len);
    retry_write(out_fd, body, strlen(body));
    retry_write(out_fd, "\n", (size_t)1);

    if (g_log_fd != -1) {
        retry_write(g_log_fd, hdr,  (size_t)hdr_len);
        retry_write(g_log_fd, body, strlen(body));
        retry_write(g_log_fd, "\n", (size_t)1);
    }
    pthread_mutex_unlock(&g_log_mutex);
}

static int init_logging(void)
{
    g_log_fd = open(LOG_FILE, O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC, 0644);
    if (g_log_fd == -1) {
        perror("open log file");
        return -1;
    }
    return 0;
}

static void close_logging(void)
{
    if (g_log_fd != -1) {
        close(g_log_fd);
        g_log_fd = -1;
    }
}

/**********************************************************************************************************************
 *  UTILITIES
 **********************************************************************************************************************/
static int secure_random(void *buf, size_t len)
{
    int fd;
    ssize_t rd;

    fd = open(URANDOM_PATH, O_RDONLY | O_CLOEXEC);
    if (fd == -1) return -1;

    rd = retry_read(fd, buf, len);
    close(fd);
    return (rd == (ssize_t)len) ? 0 : -1;
}

static void timespec_add_msec(struct timespec *ts, long msec)
{
    ts->tv_sec  += msec / 1000L;
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
    
    /* Check for potential overflow */
    if (diff_sec > LONG_MAX / 1000L || diff_sec < LONG_MIN / 1000L) {
        return (diff_sec > 0) ? LONG_MAX : LONG_MIN;
    }
    
    return diff_sec * 1000L + diff_nsec / 1000000L;
}

static int create_work_directory(void)
{
    struct stat st;
    
    if (stat(WORK_DIR, &st) == -1) {
        if (mkdir(WORK_DIR, 0755) == -1) {
            perror("mkdir");
            return -1;
        }
    } else if (!S_ISDIR(st.st_mode)) {
        errno = ENOTDIR;
        return -1;
    }
    return 0;
}

static void print_system_info(void)
{
    struct utsname uts;
    struct rusage usage;
    char hostname[HOST_NAME_SIZE];
    
    if (uname(&uts) == 0) {
        log_message(LOG_INFO, "main", "System: %s %s %s %s",
                    uts.sysname, uts.release, uts.version, uts.machine);
    }
    
    if (gethostname(hostname, sizeof hostname) == 0) {
        hostname[HOST_NAME_SIZE - 1] = '\0';
        log_message(LOG_INFO, "main", "Hostname: %s", hostname);
    }
    
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        log_message(LOG_INFO, "main", "CPU time: user=%ld.%06ld sys=%ld.%06ld",
                    usage.ru_utime.tv_sec, usage.ru_utime.tv_usec,
                    usage.ru_stime.tv_sec, usage.ru_stime.tv_usec);
    }
}

/**********************************************************************************************************************
 *  SHARED MEMORY
 **********************************************************************************************************************/
static int init_shared_memory(void)
{
    g_shared_data = calloc((size_t)1, sizeof(shared_data_t));
    if (!g_shared_data) {
        return -1;
    }
    
    if (pthread_mutex_init(&g_shared_data->mutex, NULL) != 0) {
        free(g_shared_data);
        return -1;
    }
    
    if (sem_init(&g_shared_data->sem_prod, 0, SHARED_MEM_SIZE) != 0 ||
        sem_init(&g_shared_data->sem_cons, 0, 0) != 0) {
        pthread_mutex_destroy(&g_shared_data->mutex);
        free(g_shared_data);
        return -1;
    }
    
    atomic_init(&g_shared_data->counter, 0);
    return 0;
}

static void cleanup_shared_memory(void)
{
    if (g_shared_data) {
        sem_destroy(&g_shared_data->sem_prod);
        sem_destroy(&g_shared_data->sem_cons);
        pthread_mutex_destroy(&g_shared_data->mutex);
        free(g_shared_data);
        g_shared_data = NULL;
    }
}

/**********************************************************************************************************************
 *  MEMORY MAPPED FILE
 **********************************************************************************************************************/
static int init_memory_mapped_file(void)
{
    int fd;
    
    fd = open(MMAP_FILE, O_RDWR | O_CREAT | O_CLOEXEC, 0644);
    if (fd == -1) {
        log_message(LOG_ERROR, "main", "Failed to open mmap file: %s", strerror(errno));
        return -1;
    }
    
    if (ftruncate(fd, (off_t)MMAP_SIZE) == -1) {
        close(fd);
        return -1;
    }
    
    g_mmap_addr = mmap(NULL,
                   (size_t)MMAP_SIZE,
                   PROT_READ | PROT_WRITE,
                   MAP_SHARED,
                   fd,
                   (off_t)0);
    close(fd);
    
    if (g_mmap_addr == MAP_FAILED) {
        log_message(LOG_ERROR, "main", "mmap failed: %s", strerror(errno));
        return -1;
    }
    
    g_mmap_size = MMAP_SIZE;
    memset(g_mmap_addr, 0, (size_t)MMAP_SIZE);
    return 0;
}

static void cleanup_memory_mapped_file(void)
{
    if (g_mmap_addr != MAP_FAILED) {
        munmap(g_mmap_addr, g_mmap_size);
        g_mmap_addr = MAP_FAILED;
        g_mmap_size = 0;
    }
}

/**********************************************************************************************************************
 *  FIFO (Named Pipe)
 **********************************************************************************************************************/
static int init_fifo(void)
{
    struct stat st;
    
    /* Remove existing FIFO if present */
    if (stat(FIFO_PATH, &st) == 0) {
        if (S_ISFIFO(st.st_mode)) {
            unlink(FIFO_PATH);
        }
    }
    
    /* Create FIFO */
    if (mkfifo(FIFO_PATH, 0666) == -1) {
        log_message(LOG_ERROR, "main", "Failed to create FIFO: %s", strerror(errno));
        return -1;
    }
    
    /* Open FIFO for reading in non-blocking mode */
    g_fifo_fd = open(FIFO_PATH, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if (g_fifo_fd == -1) {
        log_message(LOG_ERROR, "main", "Failed to open FIFO: %s", strerror(errno));
        unlink(FIFO_PATH);
        return -1;
    }
    
    log_message(LOG_INFO, "main", "FIFO created at %s", FIFO_PATH);
    return 0;
}

static void cleanup_fifo(void)
{
    if (g_fifo_fd != -1) {
        close(g_fifo_fd);
        g_fifo_fd = -1;
    }
    unlink(FIFO_PATH);
}

/**********************************************************************************************************************
 *  DIRECTORY MONITORING
 **********************************************************************************************************************/
static int monitor_directory(const char *path)
{
    DIR *dir;
    struct dirent *entry;
    struct stat st;
    char full_path[MAX_PATH];
    int count = 0;
    
    dir = opendir(path);
    if (!dir) {
        return -1;
    }
    
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        
        snprintf(full_path, sizeof full_path, "%s/%s", path, entry->d_name);
        
        if (stat(full_path, &st) == 0) {
            count++;
        }
    }
    
    closedir(dir);
    return count;
}

/**********************************************************************************************************************
 *  FORK/EXEC DEMONSTRATION
 **********************************************************************************************************************/
static void demonstrate_fork_exec(void)
{
    pid_t pid;
    int status;
    static char arg0[] = "echo";
    static char arg1[] = "Child process executed via fork/exec";
    static char *const exec_args[] = { arg0, arg1, NULL };
    
    pid = fork();
    if (pid == -1) {
        log_message(LOG_ERROR, "main", "fork() failed: %s", strerror(errno));
        return;
    }
    
    if (pid == 0) {
        /* Child process */
        if (execvp(exec_args[0], exec_args) == -1) {
            log_message(LOG_ERROR, "main", "execvp() failed: %s", strerror(errno));
            _exit(EXIT_FAILURE);  /* Exit child process on exec failure */
        }
    }
    
    /* Parent process */
    log_message(LOG_INFO, "main", "Created child process with PID %d", (int)pid);
    
    if (waitpid(pid, &status, 0) == pid) {
        if (WIFEXITED(status)) {
            log_message(LOG_INFO, "main", "Child process exited with status %d", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            log_message(LOG_INFO, "main", "Child process terminated by signal %d", WTERMSIG(status));
        }
    }
}

/**********************************************************************************************************************
 *  WORKER THREADS
 **********************************************************************************************************************/
static void *random_worker_thread(void *arg)
{
    thread_info_t *info = (thread_info_t *)arg;
    struct timespec next;
    unsigned char buf[32];
    long counter = 0;
    
    clock_gettime(CLOCK_MONOTONIC, &next);
    log_message(LOG_INFO, info->name, "Thread started");
    
    while (!g_terminate) {
        struct timespec now;
        long wait_ms;

        timespec_add_msec(&next, 2500L);
        
        if (secure_random(buf, sizeof buf) == 0) {
            counter++;
            atomic_fetch_add(&g_shared_data->counter, 1);
            log_message(LOG_DEBUG, info->name, "Random data generated (count: %ld)", counter);
            
            /* Write to mmap if available */
            if (g_mmap_addr != MAP_FAILED) {
                pthread_mutex_lock(&g_shared_data->mutex);
                memcpy(g_mmap_addr, buf, sizeof buf);
                pthread_mutex_unlock(&g_shared_data->mutex);
            }
            
            /* Write some data to shared buffer */
            if (sem_trywait(&g_shared_data->sem_prod) == 0) {
                pthread_mutex_lock(&g_shared_data->mutex);
                g_shared_data->buffer[g_shared_data->write_pos] = (char)(buf[0] % 256);
                g_shared_data->write_pos = (g_shared_data->write_pos + 1) % SHARED_MEM_SIZE;
                pthread_mutex_unlock(&g_shared_data->mutex);
                sem_post(&g_shared_data->sem_cons);
            }
        } else {
            log_message(LOG_WARN, info->name, "Failed to generate random data");
        }

        now = (struct timespec){0};
        clock_gettime(CLOCK_MONOTONIC, &now);
        wait_ms = timespec_sub_msec(&next, &now);
        if (wait_ms > 0) {
            struct timespec ts = {wait_ms / 1000L, (wait_ms % 1000L) * 1000000L};
            while (nanosleep(&ts, &ts) != 0 && errno == EINTR) { /* retry */ }
        }
    }
    
    log_message(LOG_INFO, info->name, "Thread exiting");
    return NULL;
}

static void *file_monitor_thread(void *arg)
{
    thread_info_t *info = (thread_info_t *)arg;
    struct timespec next;
    int prev_count = -1;
    char test_file[MAX_PATH];
    int test_fd;
    DIR            *dir;
    struct dirent  *entry;
    struct stat     st;
    
    clock_gettime(CLOCK_MONOTONIC, &next);
    log_message(LOG_INFO, info->name, "Thread started");
    
    while (!g_terminate) {
        int             count;
        size_t          used;
        struct timespec now;
        long            wait_ms;

        timespec_add_msec(&next, 3000L);

        count = monitor_directory(WORK_DIR);
        if (count >= 0 && count != prev_count) {
            log_message(LOG_INFO, info->name, "Directory %s has %d entries", WORK_DIR, count);
            prev_count = count;
        }
        
        /* Check shared memory buffer usage */
        pthread_mutex_lock(&g_shared_data->mutex);
        used = (g_shared_data->write_pos - g_shared_data->read_pos) % SHARED_MEM_SIZE;
        pthread_mutex_unlock(&g_shared_data->mutex);
        log_message(LOG_DEBUG, info->name, "Shared buffer usage: %zu/%d bytes", used, SHARED_MEM_SIZE);
        
        /* Create and remove a test file periodically */
        snprintf(test_file, sizeof test_file, "%s/test_%ld.tmp", WORK_DIR, time(NULL));
        test_fd = open(test_file, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0644);
        if (test_fd != -1) {
            const char *msg = "Test file content\n";
            retry_write(test_fd, msg, strlen(msg));
            close(test_fd);
            log_message(LOG_DEBUG, info->name, "Created test file: %s", test_file);
            
            /* Remove old test files */
            dir = opendir(WORK_DIR);
            if (dir) {
                while ((entry = readdir(dir)) != NULL) {
                    if (strstr(entry->d_name, "test_") && strstr(entry->d_name, ".tmp")) {
                        char old_file[MAX_PATH];
                        snprintf(old_file, sizeof old_file, "%s/%s", WORK_DIR, entry->d_name);
                        if (stat(old_file, &st) == 0 && (time(NULL) - st.st_mtime) > 10) {
                            unlink(old_file);
                            log_message(LOG_DEBUG, info->name, "Removed old test file: %s", entry->d_name);
                        }
                    }
                }
                closedir(dir);
            }
        }

        now = (struct timespec){0};
        clock_gettime(CLOCK_MONOTONIC, &now);
        wait_ms = timespec_sub_msec(&next, &now);
        if (wait_ms > 0) {
            struct timespec ts = {wait_ms / 1000L, (wait_ms % 1000L) * 1000000L};
            while (nanosleep(&ts, &ts) != 0 && errno == EINTR) { /* retry */ }
        }
    }
    
    log_message(LOG_INFO, info->name, "Thread exiting");
    return NULL;
}

static void *process_monitor_thread(void *arg)
{
    thread_info_t *info = (thread_info_t *)arg;
    struct timespec next;
    char fifo_buf[256];
    
    clock_gettime(CLOCK_MONOTONIC, &next);
    log_message(LOG_INFO, info->name, "Thread started");
    
    while (!g_terminate) {
        pid_t           child;
        struct rlimit   rlim;
        struct timespec now;
        long            wait_ms;
        char data = 0;

        timespec_add_msec(&next, 4000L);
        
        /* Read from FIFO if available */
        if (g_fifo_fd != -1) {
            ssize_t n = read(g_fifo_fd, fifo_buf, sizeof fifo_buf - 1);
            if (n > 0) {
                fifo_buf[n] = '\0';
                log_message(LOG_INFO, info->name, "Received from FIFO: %s", fifo_buf);
            }
        }
        
        /* Monitor child processes */
        child = waitpid(-1, NULL, WNOHANG);
        if (child > 0) {
            log_message(LOG_INFO, info->name, "Reaped child process %d", (int)child);
        }
        
        /* Consume data from shared buffer */
        if (sem_trywait(&g_shared_data->sem_cons) == 0) {
            pthread_mutex_lock(&g_shared_data->mutex);
            data = g_shared_data->buffer[g_shared_data->read_pos];
            g_shared_data->read_pos = (g_shared_data->read_pos + 1) % SHARED_MEM_SIZE;
            pthread_mutex_unlock(&g_shared_data->mutex);
            sem_post(&g_shared_data->sem_prod);
            log_message(LOG_DEBUG, info->name, "Consumed data from shared buffer: 0x%02x", (unsigned char)data);
        }
        
        /* Check process limits */
        if (getrlimit(RLIMIT_NPROC, &rlim) == 0) {
            log_message(LOG_DEBUG, info->name, "Process limit: soft=%ld hard=%ld",
                        (long)rlim.rlim_cur, (long)rlim.rlim_max);
        }
        
        now = (struct timespec){0};
        clock_gettime(CLOCK_MONOTONIC, &now);
        wait_ms = timespec_sub_msec(&next, &now);
        if (wait_ms > 0) {
            struct timespec ts = {wait_ms / 1000L, (wait_ms % 1000L) * 1000000L};
            while (nanosleep(&ts, &ts) != 0 && errno == EINTR) { /* retry */ }
        }
    }
    
    log_message(LOG_INFO, info->name, "Thread exiting");
    return NULL;
}

static void *system_info_thread(void *arg)
{
    thread_info_t *info = (thread_info_t *)arg;
    struct timespec next;
    
    clock_gettime(CLOCK_MONOTONIC, &next);
    log_message(LOG_INFO, info->name, "Thread started");
    
    while (!g_terminate) {
        struct rusage   usage;
        long            count;
        struct timespec process_time;
        struct rlimit   rlim;
        struct timespec now;
        long            wait_ms;
        unsigned char first_byte = 0;

        timespec_add_msec(&next, 10000L);
        
        /* Get process info */
        if (getrusage(RUSAGE_SELF, &usage) == 0) {
            log_message(LOG_INFO, info->name, 
                        "Process resources - RSS: %ld KB, User CPU: %ld.%06ld, Sys CPU: %ld.%06ld",
                        usage.ru_maxrss, 
                        usage.ru_utime.tv_sec, usage.ru_utime.tv_usec,
                        usage.ru_stime.tv_sec, usage.ru_stime.tv_usec);
        }
        
        /* Report shared counter */
        count = atomic_load(&g_shared_data->counter);
        log_message(LOG_INFO, info->name, "Global counter: %ld", count);
        
        /* Get process times for performance monitoring */
        process_time = (struct timespec){0};
        if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &process_time) == 0) {
            log_message(LOG_INFO, info->name, "Process CPU time: %ld.%09ld seconds", 
                        process_time.tv_sec, process_time.tv_nsec);
        }
        
        /* Get number of open file descriptors */
        rlim = (struct rlimit){0};
        if (getrlimit(RLIMIT_NOFILE, &rlim) == 0) {
            log_message(LOG_INFO, info->name, "File descriptor limit: soft=%ld hard=%ld",
                        (long)rlim.rlim_cur, (long)rlim.rlim_max);
        }
        
        /* Check memory mapped file content */
        if (g_mmap_addr != MAP_FAILED) {
            pthread_mutex_lock(&g_shared_data->mutex);
            first_byte = *((unsigned char *)g_mmap_addr);
            pthread_mutex_unlock(&g_shared_data->mutex);
            log_message(LOG_DEBUG, info->name, "First byte of mmap: 0x%02x", first_byte);
        }
        
        now = (struct timespec){0};
        clock_gettime(CLOCK_MONOTONIC, &now);
        wait_ms = timespec_sub_msec(&next, &now);
        if (wait_ms > 0) {
            struct timespec ts = {wait_ms / 1000L, (wait_ms % 1000L) * 1000000L};
            while (nanosleep(&ts, &ts) != 0 && errno == EINTR) { /* retry */ }
        }
    }
    
    log_message(LOG_INFO, info->name, "Thread exiting");
    return NULL;
}

/**********************************************************************************************************************
 *  MAIN
 **********************************************************************************************************************/
int main(void)
{
    struct pollfd fds[3];
    char line[MAX_LINE_BYTES];
    int ret = EXIT_FAILURE;
    
    /* Initialize subsystems */
    if (create_work_directory() != 0) {
        fprintf(stderr, "Failed to create work directory\n");
        return EXIT_FAILURE;
    }
    
    if (init_logging() != 0) {
        fprintf(stderr, "Failed to initialize logging\n");
        return EXIT_FAILURE;
    }
    
    log_message(LOG_INFO, "main", "Starting POSIX test-bed application (fully static build)");
    print_system_info();
    
    if (install_signal_handlers() != 0) {
        goto cleanup;
    }
    
    if (init_shared_memory() != 0) {
        log_message(LOG_ERROR, "main", "Failed to initialize shared memory");
        goto cleanup;
    }
    
    if (init_memory_mapped_file() != 0) {
        log_message(LOG_ERROR, "main", "Failed to initialize memory mapped file");
        goto cleanup;
    }
    
    if (init_fifo() != 0) {
        log_message(LOG_ERROR, "main", "Failed to initialize FIFO");
        goto cleanup;
    }
    
    /* Demonstrate fork/exec */
    demonstrate_fork_exec();
    
    /* Initialize threads */
    memset(g_threads, 0, sizeof g_threads);
    
    g_threads[0].thread_id = 0;
    strncpy(g_threads[0].name, "random", sizeof(g_threads[0].name) - 1);
    g_threads[0].name[sizeof(g_threads[0].name) - 1] = '\0';
    atomic_store(&g_threads[0].active, 1);
    
    g_threads[1].thread_id = 1;
    strncpy(g_threads[1].name, "monitor", sizeof(g_threads[1].name) - 1);
    g_threads[1].name[sizeof(g_threads[1].name) - 1] = '\0';
    atomic_store(&g_threads[1].active, 1);
    
    g_threads[2].thread_id = 2;
    strncpy(g_threads[2].name, "process", sizeof(g_threads[2].name) - 1);
    g_threads[2].name[sizeof(g_threads[2].name) - 1] = '\0';
    atomic_store(&g_threads[2].active, 1);
    
    g_threads[3].thread_id = 3;
    strncpy(g_threads[3].name, "sysinfo", sizeof(g_threads[3].name) - 1);
    g_threads[3].name[sizeof(g_threads[3].name) - 1] = '\0';
    atomic_store(&g_threads[3].active, 1);
    
    /* Create threads */
    if (pthread_create(&g_threads[0].tid, NULL, random_worker_thread, &g_threads[0]) != 0 ||
        pthread_create(&g_threads[1].tid, NULL, file_monitor_thread, &g_threads[1]) != 0 ||
        pthread_create(&g_threads[2].tid, NULL, process_monitor_thread, &g_threads[2]) != 0 ||
        pthread_create(&g_threads[3].tid, NULL, system_info_thread, &g_threads[3]) != 0) {
        log_message(LOG_ERROR, "main", "Failed to create threads");
        goto cleanup;
    }
    
    /* Setup poll descriptors */
    fds[0].fd = STDIN_FILENO;  fds[0].events = POLLIN;
    fds[1].fd = g_sig_pipe[0]; fds[1].events = POLLIN;
    fds[2].fd = g_fifo_fd;     fds[2].events = POLLIN;
    
    log_message(LOG_INFO, "main", "System ready - Press Ctrl-C to exit, SIGHUP to reload");
    log_message(LOG_INFO, "main", "Write to FIFO: echo 'message' > %s", FIFO_PATH);
    
    /* Main event loop */
    while (!g_terminate) {
        int poll_ret = poll(fds, 3, (int)TICK_MSEC);
        
        if (poll_ret == -1) {
            if (errno == EINTR) continue;
            log_message(LOG_ERROR, "main", "poll failed: %s", strerror(errno));
            break;
        }
        
        if (poll_ret == 0) {
            log_message(LOG_DEBUG, "main", "Main loop tick (%ld ms)", TICK_MSEC);
            continue;
        }
        
        /* Handle signals */
        if (fds[1].revents & POLLIN) {
            char sigbuf[8];
            (void)retry_read(g_sig_pipe[0], sigbuf, sizeof sigbuf);
            
            if (g_reload_conf) {
                log_message(LOG_INFO, "main", "Configuration reload requested (SIGHUP)");
                g_reload_conf = 0;
            }
        }
        
        /* Handle stdin */
        if (fds[0].revents & POLLIN) {
            ssize_t n = retry_read(STDIN_FILENO, line, sizeof line - 1);
            if (n > 0) {
                line[n] = '\0';
                /* Remove newline if present */
                if (n > 0 && line[n-1] == '\n') {
                    line[n-1] = '\0';
                }
                log_message(LOG_INFO, "stdin", "%s", line);
            } else if (n == 0) {
                log_message(LOG_INFO, "main", "EOF on stdin");
                g_terminate = 1;
            }
        }
    }
    
    ret = EXIT_SUCCESS;
    
cleanup:
    log_message(LOG_INFO, "main", "Shutting down...");
    
    /* Wait for threads */
    for (int i = 0; i < MAX_THREADS; i++) {
        if (g_threads[i].tid) {
            pthread_join(g_threads[i].tid, NULL);
        }
    }
    
    /* Cleanup resources */
    cleanup_fifo();
    cleanup_memory_mapped_file();
    cleanup_shared_memory();
    
    if (g_sig_pipe[0] != -1) close(g_sig_pipe[0]);
    if (g_sig_pipe[1] != -1) close(g_sig_pipe[1]);
    
    log_message(LOG_INFO, "main", "Graceful shutdown complete");
    close_logging();
    
    return ret;
}