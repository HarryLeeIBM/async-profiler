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
#include "os.h"

class AixThreadList : public ThreadList {
  private:
    pthread_t* _threads;
    int _count;
    int _index;

  public:
    AixThreadList() {
        _threads = NULL;
        _count = 0;
        _index = -1;
        update();
    }

    ~AixThreadList() {
        free(_threads);
    }

    int next() {
        if (++_index < _count) {
            return (int)_threads[_index];
        }
        return 0;
    }

    void update() {
        free(_threads);
        _threads = NULL;
        _count = 0;
        _index = -1;
        
        // Get the maximum number of threads
        int max_threads = sysconf(_SC_THREAD_THREADS_MAX);
        if (max_threads <= 0) {
            max_threads = 1024;  // Fallback value
        }
        
        _threads = (pthread_t*)malloc(max_threads * sizeof(pthread_t));
        if (_threads == NULL) {
            return;
        }
        
        // Use pthread_getthrds_np to enumerate all threads in the process
        struct __pthrdsinfo* thread_info = (__pthrdsinfo*)malloc(sizeof(__pthrdsinfo));
        if (thread_info == NULL) {
            free(_threads);
            _threads = NULL;
            return;
        }
        
        void* reg_buf = malloc(1024);  // Buffer for register values
        int reg_buf_size = 1024;
        pthread_t thread = 0;          // Will be updated by pthread_getthrds_np
        int index = 0;
        
        // Loop until we've found all threads or reached max_threads
        while (_count < max_threads) {
            int ret = pthread_getthrds_np(&thread, PTHRDSINFO_QUERY_ALL,
                                         thread_info, sizeof(__pthrdsinfo),
                                         reg_buf, &reg_buf_size);
            
            if (thread == 0) {
                // No more threads or error
                break;
            }
            
            // Store the thread ID
            _threads[_count++] = thread;
        }
        
        free(thread_info);
        free(reg_buf);
        
        // If no threads were found, at least include the current thread
        if (_count == 0) {
            _threads[0] = pthread_self();
            _count = 1;
        }
    }
};

JitWriteProtection::JitWriteProtection(bool enable) {


}

JitWriteProtection::~JitWriteProtection() {
}


static SigAction installed_sigaction[32];

const size_t OS::page_size = sysconf(_SC_PAGESIZE);
const size_t OS::page_mask = OS::page_size - 1;


u64 OS::nanotime() {
    return 0;
}

u64 OS::micros() {
    return 0;
}

void OS::sleep(u64 nanos) {
}

u64 OS::overrun(siginfo_t* siginfo) {
    return 0;
}

u64 OS::processStartTime() {
    static u64 start_time = 0;

    return start_time;
}

u64 OS::hton64(u64 x) {
    return 0;
}

u64 OS::ntoh64(u64 x) {
    return 0;
}

int OS::getMaxThreadId() {
    return 0x7fffffff;
}

int OS::processId() {
    return 0;
}

int OS::threadId() {
  
    return (int)0;
}

const char* OS::schedPolicy(int thread_id) {
    // Not used on macOS
    return "SCHED_OTHER";
}

bool OS::threadName(int thread_id, char* name_buf, size_t name_len) {
   
    return true;
}

ThreadState OS::threadState(int thread_id) {
    return ThreadState{};
}

u64 OS::threadCpuTime(int thread_id) {
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
    return NULL;
}

void OS::safeFree(void* addr, size_t size) {
}

bool OS::getCpuDescription(char* buf, size_t size) {
    return false;
}

int OS::getCpuCount() {
    return false;
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
    
}

void OS::freePageCache(int fd, off_t start_offset) {
    // Not supported on macOS
}

#endif // __AIX__


