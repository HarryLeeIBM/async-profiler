/*
 * Copyright The async-profiler authors
 * SPDX-License-Identifier: Apache-2.0
 */

 #include "arch.h"
 #include "testRunner.hpp"
 #include "os.h"
 #include <stdio.h>
 
 TEST_CASE(OS_test_time) {
    bool os_type = OS::isLinux();
    CHECK_EQ(os_type, true);
 }
