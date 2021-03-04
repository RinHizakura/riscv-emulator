#include "uart.h"
#include <string.h>

//
#define uart_reg(uart, addr) uart->reg[addr - UART_BASE]

void init_uart(riscv_uart *uart)
{
    memset(uart->reg, 0, sizeof(UART_SIZE));
    uart->is_interrupt = false;
    // transmitter hold register is empty at first
    uart_reg(uart, UART_LSR) |= UART_LSR_TX;
}

uint64_t read_uart(riscv_uart *uart,
                   uint64_t addr,
                   uint8_t size,
                   riscv_exception *exc)
{
    if (size != 8) {
        exc->exception = LoadAccessFault;
        return -1;
    }

    switch (addr) {
    case UART_RHR:
        // no data in receive holding register after reading
        uart_reg(uart, UART_LSR) &= ~UART_LSR_RX;
        return uart_reg(uart, addr);
    default:
        return uart_reg(uart, addr);
    }
}

bool write_uart(riscv_uart *uart,
                uint64_t addr,
                uint8_t size,
                uint64_t value,
                riscv_exception *exc)
{
    if (size != 8) {
        exc->exception = StoreAMOAccessFault;
        return false;
    }

    switch (addr) {
    case UART_THR:
        /* Note: The case UART_LSR_TX == 0 isn't emulated. It means that our
         * emulated UART doesn't drop the character.
         */
        printf("%c", (char) (value & 0xff));
        break;
    default:
        uart_reg(uart, addr) = value & 0xff;
        break;
    }

    return true;
}
