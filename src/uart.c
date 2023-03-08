#include "uart.h"

#include <poll.h>
#include <string.h>
#include <unistd.h>

bool init_uart(riscv_uart *uart)
{
    memset(&uart->reg, 0, sizeof(uart->reg));
    // transmitter hold register is empty at first
    uart->reg.lsr |= (UART_LSR_TEMT | UART_LSR_THRE);
    /*  BIT 6-7: are set to "1" in 16550 mode */
    uart->reg.isr |= 0xc0;


    uart->is_interrupt = false;
    uart->infd = STDIN_FILENO;
    fifo_init(&uart->rx_buf);

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
    if (uart->reg.lsr & UART_LSR_DR)
        return;

    while (!fifo_is_full(&uart->rx_buf) && uart_readable(uart, 0)) {
        char c;
        if (read(uart->infd, &c, 1) == -1)
            break;

        if (!fifo_put(&uart->rx_buf, c))
            break;

        uart->reg.lsr |= UART_LSR_DR;
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
    case UART_THR:  // UART_DLL
        if (uart->reg.lcr & UART_LCR_DLAB) {
            ret_value = uart->reg.dll;
        } else {
            /* FIXME: What's the correct action if there's no
             * data in RX buffer? */
            if (!fifo_get(&uart->rx_buf, ret_value))
                break;

            if (fifo_is_empty(&uart->rx_buf))
                uart->reg.lsr &= ~UART_LSR_DR;
        }
        break;
    case UART_IER:  // UART_DLM
        if (uart->reg.lcr & UART_LCR_DLAB)
            ret_value = uart->reg.dlm;
        else
            ret_value = uart->reg.ier;
        break;
    case UART_ISR:
        ret_value = uart->reg.isr;
        break;
    case UART_LCR:
        ret_value = uart->reg.lcr;
        break;
    case UART_MCR:
        ret_value = uart->reg.mcr;
        break;
    case UART_LSR:
        ret_value = uart->reg.lsr;
        break;
    case UART_MSR:
        ret_value = uart->reg.msr;
        break;
    case UART_SCR:
        ret_value = uart->reg.scr;
        break;
    default:
        break;
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

    value &= 0xff;

    switch (addr) {
    case UART_THR:  // UART_DLL
        if (uart->reg.lcr & UART_LCR_DLAB) {
            uart->reg.dll = value;
        } else {
            putchar((char) value);
            fflush(stdout);
        }
        break;
    case UART_IER:  // UART_DLM
        if (uart->reg.lcr & UART_LCR_DLAB) {
            uart->reg.dlm = value;
        } else {
            uart->reg.ier = value;
            if (uart->reg.ier & UART_IER_RDI)
                uart->is_interrupt = true;
        }
        break;
    case UART_FCR:
        uart->reg.fcr = value;
        break;
    case UART_LCR:
        uart->reg.lcr = value;
        break;
    case UART_MCR:
        uart->reg.mcr = value;
        break;
    case UART_SCR:
        uart->reg.scr = value;
        break;
    default:
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
