#include "uart.h"

#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#define uart_reg(uart, addr) uart->reg[addr - UART_BASE]

bool init_uart(riscv_uart *uart)
{
    memset(&uart->reg[0], 0, UART_SIZE * sizeof(uint8_t));
    uart->is_interrupt = false;
    uart->infd = STDIN_FILENO;
    fifo_init(&uart->rx_buf);

    // transmitter hold register is empty at first
    uart_reg(uart, UART_LSR) |= UART_LSR_TX;

    return true;
}

static int uart_readable(riscv_uart *uart, int timeout)
{
    struct pollfd pollfd = (struct pollfd){
        .fd = uart->infd,
        .events = POLLIN,
    };
    return (poll(&pollfd, 1, timeout) > 0) && (pollfd.revents & POLLIN);
}

void tick_uart(riscv_uart *uart)
{
    if (uart_reg(uart, UART_LSR) & UART_LSR_RX)
        return;

    while (!fifo_is_full(&uart->rx_buf) && uart_readable(uart, 0)) {
        char c;
        if (read(uart->infd, &c, 1) == -1)
            break;

        if (!fifo_put(&uart->rx_buf, c))
            break;

        uart_reg(uart, UART_LSR) |= UART_LSR_RX;
        uart->is_interrupt = true;
    }
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

    uint64_t ret_value = -1;

    switch (addr) {
    case UART_RHR:
        if (!fifo_get(&uart->rx_buf, ret_value)) {
            /* FIXME: What's the correct action if there's no
             * data in RX buffer? */
            break;
        }

        if (fifo_is_empty(&uart->rx_buf))
            uart_reg(uart, UART_LSR) &= ~UART_LSR_RX;
        break;
    default:
        ret_value = uart_reg(uart, addr);
    }

    return ret_value;
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
        fflush(stdout);
        break;
    case UART_IER:
        if ((value & UART_IER_THR_EMPTY_INT) != 0) {
            __atomic_store_n(&uart->is_interrupt, true, __ATOMIC_SEQ_CST);
        }
        uart_reg(uart, addr) = value & 0xff;
        break;
    case UART_FCR:
        /* Avoid to overwrite the register at address 0x2, which is a
         * shared address with UART_ISR.
         *
         * TODO: Implement the behavior for FIFO control */
        break;
    default:
        uart_reg(uart, addr) = value & 0xff;
        break;
    }

    return true;
}

bool uart_is_interrupt(riscv_uart *uart)
{
    bool ret = uart->is_interrupt;
    uart->is_interrupt = false;
    return ret;
}

void free_uart(__attribute__((unused)) riscv_uart *uart)
{
    /* Do nothing currently */
}
