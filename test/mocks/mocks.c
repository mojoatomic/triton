/**
 * Mock implementations for unit testing
 *
 * This file is compiled with unit tests to provide
 * implementations of functions that would normally
 * be provided by the Pico SDK or hardware.
 */

#include "mock_pico.h"

// Define I2C_PORT for tests if not already defined
#ifndef I2C_PORT
#define I2C_PORT i2c0
#endif

// Mock printf for embedded code (if needed)
// In unit tests, we want actual printf to work
