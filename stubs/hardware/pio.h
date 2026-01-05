#ifndef STUB_HARDWARE_PIO_H
#define STUB_HARDWARE_PIO_H

#include <stdint.h>
#include <stdbool.h>

typedef unsigned int uint;

typedef struct pio_inst* PIO;

extern PIO pio0;
extern PIO pio1;

typedef struct pio_program {
    const uint16_t* instructions;
    uint length;
    int origin;
} pio_program_t;

uint pio_add_program(PIO pio, const pio_program_t* program);
int pio_claim_unused_sm(PIO pio, bool required);
void pio_sm_unclaim(PIO pio, uint sm);

bool pio_sm_is_rx_fifo_empty(PIO pio, uint sm);
uint32_t pio_sm_get_blocking(PIO pio, uint sm);

#endif // STUB_HARDWARE_PIO_H
