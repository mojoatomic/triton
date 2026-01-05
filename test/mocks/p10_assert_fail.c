/**
 * Unit test support
 * p10_assert_fail.c - Host-side Power-of-10 assertion failure handler
 */

#include "types.h"

#include <stdio.h>
#include <stdlib.h>

void p10_assert_fail(const char* file, int line, const char* cond) {
    if ((file == NULL) || (cond == NULL)) {
        fprintf(stderr, "P10_ASSERT failed (unknown location)\n");
        abort();
    }

    fprintf(stderr, "P10_ASSERT failed: %s:%d: %s\n", file, line, cond);
    abort();
}
