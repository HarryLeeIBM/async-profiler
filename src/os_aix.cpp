/*
 * Copyright The async-profiler authors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef __AIX__

#include <stdlib.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/times.h>
#include <time.h>
#include <unistd.h>

#include <procinfo.h>
#include <sys/types.h>
#include <sys/thread.h> 
#include "os.h"


#define MAX_REG_BUFFER 1024
class AixThreadList : public ThreadList {
private:
    int *_thread_array;
    u32 _capacity;

    void addThread(int thread_id) {
        if (_count >= _capacity) {
            _capacity = _count * 2;
            _thread_array = (int *)realloc(_thread_array, _capacity * sizeof(int));
        }
        _thread_array[_count++] = thread_id;
    }

    void fillThreadArray() {
        struct __pthrdsinfo thread_info;
        unsigned char reg_buf[MAX_REG_BUFFER];

        int reg_buf_size = MAX_REG_BUFFER;
        pthread_t thread = 0;
        int index = 0;
        while (true) {
            // Get pthread ID
            int ret = pthread_getthrds_np(&thread, PTHRDSINFO_QUERY_ALL,
                                          &thread_info, sizeof(__pthrdsinfo),
                                          reg_buf, &reg_buf_size);

            if (thread == 0 || ret != 0) {
                break;
            }
            addThread(thread_info.__pi_tid);
        }
    }

public:
    AixThreadList() : ThreadList() {
        _capacity = 128;
        _thread_array = (int *)malloc(_capacity * sizeof(int));
        fillThreadArray();
    }

    ~AixThreadList() {
        free(_thread_array);
    }

    int next() {
        return _thread_array[_index++];
    }

    void update() {
        _index = _count = 0;
        fillThreadArray();
    }
};

JitWriteProtection::JitWriteProtection(bool enable) {
}

JitWriteProtection::~JitWriteProtection() {
}


static SigAction installed_sigaction[32];

const size_t OS::page_size = sysconf(_SC_PAGESIZE);
const size_t OS::page_mask = OS::page_size - 1;


u64 OS::nanotime()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (u64)ts.tv_sec * 1000000000 + ts.tv_nsec;
}

u64 OS::micros()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (u64)tv.tv_sec * 1000000 + tv.tv_usec;
}

void OS::sleep(u64 nanos)
{
    struct timespec ts = {(time_t)(nanos / 1000000000), (long)(nanos % 1000000000)};
    nanosleep(&ts, NULL);
}
u64 OS::overrun(siginfo_t* siginfo) {
    return 0;
}

u64 OS::processStartTime() {
    static u64 start_time = 0;

    if (start_time == 0) {
        // On AIX, we can use getprocs64() to get process information
        struct procentry64 proc_info;
        pid_t pid = getpid();
        
        // Initialize the id field to our process ID
        proc_info.pi_pid = pid;
        
        // Get process information
        if (getprocs64(&proc_info, sizeof(proc_info), NULL, 0, &pid, 1) == 1) {
            // pi_start contains the process start time in seconds since epoch
            // Convert to milliseconds for consistency with other platforms
            start_time = (u64)proc_info.pi_start * 1000;
        } else {
            // Fallback: use current time
            struct timeval tv;
            gettimeofday(&tv, NULL);
            start_time = ((u64)tv.tv_sec * 1000) + (tv.tv_usec / 1000);
        }
    }

    return start_time;
}


u64 OS::hton64(u64 x) {
    return x;
}

u64 OS::ntoh64(u64 x) {
    return x;
}

int OS::getMaxThreadId() {
    return 0x7fffffff;
}

int OS::processId() {
    static const int self_pid = getpid();

    return self_pid;
}

int OS::threadId() {
    tid_t kernel_tid = thread_self();
    return (int)kernel_tid;
}
const char* OS::schedPolicy(int thread_id) {
    // Not used on macOS
    return "SCHED_OTHER";
}

bool OS::threadName(int thread_id, char* name_buf, size_t name_len) {
   
    return true;
}

ThreadState OS::threadState(int thread_id) {
    ThreadState state = THREAD_UNKNOWN; 
    
    // Convert thread_id to pthread_t
    pthread_t thread = (pthread_t)thread_id;
    
    // Use pthread_getthrds_np to get thread information
    struct __pthrdsinfo thread_info;
    void* reg_buf = malloc(1024);  // Buffer for register values
    int reg_buf_size = 1024;
    
    int ret = pthread_getthrds_np(&thread, PTHRDSINFO_QUERY_TID,
                                 &thread_info, sizeof(__pthrdsinfo),
                                 reg_buf, &reg_buf_size);
    
    if (ret == 0) {
        switch (thread_info.__pi_state) {
            case PTHRDSINFO_STATE_SLEEP:
                state = THREAD_SLEEPING;  // Sleeping
                break;
            case PTHRDSINFO_STATE_RUN:
                state = THREAD_RUNNING;  // Running
                break;
        }
    }
    
    free(reg_buf);
    return state;
}

u64 OS::threadCpuTime(int thread_id) {
    clockid_t thread_cpu_clock;
    if (thread_id) {
        thread_cpu_clock = ((~(unsigned int)(thread_id)) << 3) | 6;  // CPUCLOCK_SCHED | CPUCLOCK_PERTHREAD_MASK
    } else {
        thread_cpu_clock = CLOCK_THREAD_CPUTIME_ID;
    }

    struct timespec ts;
    if (clock_gettime(thread_cpu_clock, &ts) == 0) {
        return (u64)ts.tv_sec * 1000000000 + ts.tv_nsec;
    }
    return 0;
}

ThreadList* OS::listThreads() {
    return new AixThreadList();
}

bool OS::isLinux() {
    return false;
}

bool OS::isMusl() {
    return false;
}

SigAction OS::installSignalHandler(int signo, SigAction action, SigHandler handler) {
    struct sigaction sa;
    struct sigaction oldsa;
    sigemptyset(&sa.sa_mask);

    if (handler != NULL) {
        sa.sa_handler = handler;
        sa.sa_flags = 0;
    } else {
        sa.sa_sigaction = action;
        sa.sa_flags = SA_SIGINFO | SA_RESTART;
        if (signo > 0 && signo < sizeof(installed_sigaction) / sizeof(installed_sigaction[0])) {
            installed_sigaction[signo] = action;
        }
  }

    sigaction(signo, &sa, &oldsa);
    return oldsa.sa_sigaction;
}

SigAction OS::replaceCrashHandler(SigAction action) {
    struct sigaction sa;
    sigaction(SIGBUS, NULL, &sa);
    SigAction old_action = sa.sa_sigaction;
    sa.sa_sigaction = action;
    sigaction(SIGBUS, &sa, NULL);
    return old_action;
}

int OS::getProfilingSignal(int mode) {
    static int preferred_signals[2] = {SIGPROF, SIGVTALRM};

    const u64 allowed_signals =
        1ULL << SIGPROF | 1ULL << SIGVTALRM | 1ULL << SIGEMT | 1ULL << SIGSYS;

    int& signo = preferred_signals[mode];
    int initial_signo = signo;
    int other_signo = preferred_signals[1 - mode];

    do {
        struct sigaction sa;
        if ((allowed_signals & (1ULL << signo)) != 0 && signo != other_signo && sigaction(signo, NULL, &sa) == 0) {
            if (sa.sa_handler == SIG_DFL || sa.sa_handler == SIG_IGN || sa.sa_sigaction == installed_sigaction[signo]) {
                return signo;
            }
        }
    } while ((signo = (signo + 1) & 31) != initial_signo);

    return signo;
}

bool OS::sendSignalToThread(int thread_id, int signo) {
    return 0;
}

void* OS::safeAlloc(size_t size) {
    // mmap() is not guaranteed to be async signal safe, but in practice, it is.
    // There is no a reasonable alternative anyway.
    void* result = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (result == MAP_FAILED) {
        return NULL;
    }
    return result;
}

void OS::safeFree(void* addr, size_t size) {
    munmap(addr, size);
}

bool OS::getCpuDescription(char* buf, size_t size) {
    return false;
}

int OS::getCpuCount() {
    return 1; 
}

u64 OS::getProcessCpuTime(u64* utime, u64* stime) {
    return 0;
}

u64 OS::getTotalCpuTime(u64* utime, u64* stime) {
    return 0;
}

int OS::createMemoryFile(const char* name) {
    // Not supported on AIX
    return -1;
}

void OS::copyFile(int src_fd, int dst_fd, off_t offset, size_t size) {
    char* buf = (char*)mmap(NULL, size + offset, PROT_READ, MAP_PRIVATE, src_fd, 0);
    if (buf == NULL) {
        return;
    }

    while (size > 0) {
        ssize_t bytes = write(dst_fd, buf + offset, size < 262144 ? size : 262144);
        if (bytes <= 0) {
            break;
        }
        offset += (size_t)bytes;
        size -= (size_t)bytes;
    }

    munmap(buf, offset);
}
void OS::freePageCache(int fd, off_t start_offset) {
    // Not supported on AIX 
}

#endif // __AIX__


