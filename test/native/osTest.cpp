/*
 * Copyright The async-profiler authors
 * SPDX-License-Identifier: Apache-2.0
 */

 #include "arch.h"
 #include "testRunner.hpp"
 #include "os.h"
 #include <stdio.h>

void* thread_function(void* arg) {
    printf("Thread %d running\n", (unsigned long)pthread_self());
    //sleep for 100ms
    usleep(100000);
    printf("Thread %d exiting\n",(unsigned long)pthread_self());    
    return NULL;
}

void create_test_thread_list() {
    // spawn 10 threads
    for (int i = 0; i < 10; i++) {
        pthread_t thread;
        pthread_create(&thread, NULL, thread_function, NULL);
    }  
}

TEST_CASE(Os_test_thread_list) {
    create_test_thread_list();

    ThreadList* thread_list =OS::listThreads(); 
    thread_list->update();
    thread_list->next();
}


 
TEST_CASE(OS_test_time) {
    bool os_type = OS::isLinux();
    CHECK_EQ(os_type, false);
 }
