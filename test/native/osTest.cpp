/*
 * Copyright The async-profiler authors
 * SPDX-License-Identifier: Apache-2.0
 */

 #include "arch.h"
 #include "testRunner.hpp"
 #include "os.h"
 #include <stdio.h>
#include <pthread.h>
void* thread_function(void* arg) {
    printf("Thread %d running\n", OS::threadId());
    usleep(500000);
    printf("Thread %d exiting\n",OS::threadId());
    return NULL;
}

void create_test_thread_list() {
    for (int i = 0; i < 10; i++) {
        pthread_t thread;
        pthread_create(&thread, NULL, thread_function, NULL);
    }  
}

TEST_CASE(Os_test_thread_list) {
    create_test_thread_list();

    ThreadList* thread_list =OS::listThreads(); 
    printf("list count = %d\n", thread_list->count());
    //thread_list->update();
    while(true) {
        int id = thread_list->next();
        if (id == 0)
          break;
        printf("thread id = %d\n", id);
    }
    delete thread_list;
}


 
TEST_CASE(OS_test_time) {
    bool os_type = OS::isLinux();
#ifdef __linux__
    CHECK_EQ(os_type, true);
#else
    CHECK_EQ(os_type, false);
#endif
 }

