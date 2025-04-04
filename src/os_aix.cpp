/*
 * Copyright The async-profiler authors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef __AIX__

#include <pthread.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/times.h>
#include <time.h>
#include <unistd.h>
#include "os.h"

class AixThreadList : public ThreadList {

  public:
    AixThreadList() {;
    }

    ~AixThreadList() {
    }

    int next() {
        return 0;
    }

    void update() {
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


