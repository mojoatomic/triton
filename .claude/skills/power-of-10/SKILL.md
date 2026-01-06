---
name: power-of-10
description: NASA/JPL Power of 10 coding rules for safety-critical C code. Use when writing embedded C firmware, safety-critical systems, aerospace code, or any project requiring high reliability. Enforces static analysis, bounded loops, no dynamic memory.
---

# Power of 10 Coding Standards

Rules derived from NASA/JPL's "Power of 10" by Gerard Holzmann. These rules enable static analysis and prove code correctness for safety-critical systems.

## The Ten Rules

### Rule 1: Simple Control Flow

No `goto`, `setjmp`, `longjmp`, or direct/indirect recursion.

```c
// FORBIDDEN
void bad_recursive(int n) {
    if (n > 0) bad_recursive(n - 1);  // Recursion
}

void bad_goto(void) {
    goto cleanup;  // goto
cleanup:
    return;
}

// ALLOWED - use iteration
void good_iterative(int n) {
    for (int i = n; i > 0; i--) {
        do_work(i);
    }
}
```

### Rule 2: Fixed Loop Bounds

All loops must have statically provable upper bounds. No unbounded `while(1)` except in main loop with watchdog.

```c
// FORBIDDEN
while (condition) {  // Unknown iterations
    do_work();
}

// ALLOWED
#define MAX_RETRIES 10
for (uint32_t i = 0; i < MAX_RETRIES; i++) {
    if (try_operation()) break;
}

// ALLOWED - main loop with watchdog
while (1) {
    watchdog_update();
    safety_monitor_run();
}
```

### Rule 3: No Dynamic Memory After Init

No `malloc`, `free`, `calloc`, `realloc` in runtime code. Static allocation only.

```c
// FORBIDDEN
void bad_dynamic(void) {
    char* buf = malloc(1024);  // Dynamic allocation
    // ...
    free(buf);
}

// ALLOWED - static allocation
static char s_buffer[1024];

void good_static(void) {
    memset(s_buffer, 0, sizeof(s_buffer));
    // Use s_buffer
}
```

### Rule 4: Short Functions

Maximum 60 lines per function (excluding comments and blank lines). One screen of code.

```c
// If function exceeds 60 lines, split it:
static void process_phase_one(Context_t* ctx);
static void process_phase_two(Context_t* ctx);
static void process_phase_three(Context_t* ctx);

void process_all(Context_t* ctx) {
    process_phase_one(ctx);
    process_phase_two(ctx);
    process_phase_three(ctx);
}
```

### Rule 5: Assertions

Minimum 2 assertions per function. Assert preconditions, invariants, and postconditions.

```c
#define P10_ASSERT(cond) do { \
    if (!(cond)) { \
        p10_assert_fail(__FILE__, __LINE__, #cond); \
    } \
} while(0)

error_t read_sensor(int channel, uint16_t* value) {
    P10_ASSERT(channel >= 0);
    P10_ASSERT(channel < MAX_CHANNELS);
    P10_ASSERT(value != NULL);
    
    // Implementation...
    
    P10_ASSERT(*value <= MAX_SENSOR_VALUE);
    return ERR_NONE;
}
```

### Rule 6: Minimal Variable Scope

Declare variables at the innermost scope possible. Initialize at declaration.

```c
// FORBIDDEN
int i;
int result;
// ... 50 lines later
for (i = 0; i < 10; i++) {
    result = compute(i);
}

// ALLOWED
for (int i = 0; i < 10; i++) {
    int result = compute(i);
    process(result);
}
```

### Rule 7: Check All Return Values

Every function return must be checked. Use `MUST_CHECK` attribute.

```c
#define MUST_CHECK __attribute__((warn_unused_result))

MUST_CHECK error_t do_operation(void);

// FORBIDDEN
do_operation();  // Ignoring return value

// ALLOWED
error_t err = do_operation();
if (err != ERR_NONE) {
    handle_error(err);
    return err;
}
```

### Rule 8: Limited Preprocessor

Use preprocessor only for:
- Include guards
- Compile-time constants
- Conditional compilation (sparingly)

```c
// ALLOWED
#ifndef MODULE_H
#define MODULE_H
#define MAX_SIZE 256
#ifdef DEBUG
    #define LOG(msg) printf(msg)
#else
    #define LOG(msg)
#endif
#endif

// FORBIDDEN - complex macro logic
#define COMPUTE(x, y) ((x) > (y) ? (x) * 2 : (y) / 2)
```

### Rule 9: Limited Pointer Dereferencing

Maximum 2 levels of dereferencing. No `***ptr`.

```c
// FORBIDDEN
void bad_pointers(int*** ppp) {
    ***ppp = 42;  // Three levels
}

// ALLOWED
void good_pointers(int** pp) {
    **pp = 42;  // Two levels maximum
}

// Better - avoid deep nesting
typedef struct {
    int* data;
} Container_t;

void best_approach(Container_t* container) {
    *(container->data) = 42;  // Clear, single logical dereference
}
```

### Rule 10: Compile Clean

Code must compile with maximum warnings enabled and zero warnings.

```bash
# Required compiler flags
CFLAGS = -Wall -Wextra -Werror -Wpedantic \
         -Wconversion -Wshadow -Wstrict-prototypes \
         -Wmissing-prototypes -Wold-style-definition
```

## Error Handling Pattern

```c
typedef enum {
    ERR_NONE = 0,
    ERR_INVALID_PARAM,
    ERR_TIMEOUT,
    ERR_I2C,
    ERR_OVERFLOW
} error_t;

#define RETURN_IF_ERROR(expr) do { \
    error_t _err = (expr); \
    if (_err != ERR_NONE) return _err; \
} while(0)

error_t complex_operation(void) {
    P10_ASSERT(initialized);
    
    RETURN_IF_ERROR(step_one());
    RETURN_IF_ERROR(step_two());
    RETURN_IF_ERROR(step_three());
    
    return ERR_NONE;
}
```

## Static Analysis Requirements

Run before every commit:

```bash
# cppcheck
cppcheck --enable=all --error-exitcode=1 src/

# Custom P10 checker (if available)
python3 ci/p10_check.py src/

# Compiler warnings as errors
make CFLAGS="-Wall -Werror"
```

## Documented Exceptions

When a rule must be violated, document explicitly:

```c
// P10 EXCEPTION: Rule 2 (bounded loops)
// Justification: Main safety loop must run indefinitely.
// Mitigation: Hardware watchdog ensures termination on fault.
// Reviewed: 2026-01-05
while (1) {
    watchdog_update();
    safety_monitor_run();
}
```

## File Header Template

```c
/**
 * [Module Name]
 * [filename] - [brief description]
 *
 * Power of 10 compliant
 * 
 * Exceptions: [List any P10 exceptions or "None"]
 */
```

## Summary

These rules exist to enable static verification of code correctness. Every rule has a purpose: enabling tools to prove your code terminates, doesn't leak memory, handles all errors, and behaves predictably. Do not circumvent them without documented justification and review.
