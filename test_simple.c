#include <stdint.h>
#include <stdbool.h>

int test_function(int x) {
    if (x < 0) return -1;

    int buffer[10];
    buffer[x % 10] = x;  // Safe due to modulo

    return buffer[0];
}

int main() {
    int result = test_function(5);
    return result >= 0 ? 0 : 1;
}