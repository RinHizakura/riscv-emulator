#include "uart.h"

#include <poll.h>
#include <string.h>
#include <unistd.h>

static void uart_update_irq(riscv_uart *uart)
{
    uint8_t isr = UART_ISR_NO_INT;

    /* If enable receiver data interrupt and receiver data ready */
    if ((uart->reg.ier & UART_IER_RDI) && (uart->reg.lsr & UART_LSR_DR))
        isr = UART_ISR_RDI;
    /* If enable transmiter data interrupt and transmiter empty */
    else if ((uart->reg.ier & UART_IER_THRI) && (uart->reg.lsr & UART_LSR_TEMT))
        isr = UART_ISR_THRI;

    uart->reg.isr = (0xc0 | isr);

    uart->is_interrupted = (isr == UART_ISR_NO_INT) ? false : true;
}

bool init_uart(riscv_uart *uart)
{
    memset(&uart->reg, 0, sizeof(uart->reg));
    // transmitter hold register is empty at first
    uart->reg.lsr |= (UART_LSR_TEMT | UART_LSR_THRE);
    /*  BIT 6-7: are set to "1" in 16550 mode */
    uart->reg.isr |= (0xc0 | UART_ISR_NO_INT);


    uart->is_interrupted = false;
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
        uart_update_irq(uart);
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

            if (fifo_is_empty(&uart->rx_buf)) {
                uart->reg.lsr &= ~UART_LSR_DR;
                uart_update_irq(uart);
            }
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
            uart_update_irq(uart);
        }
        break;
    case UART_IER:  // UART_DLM
        if (uart->reg.lcr & UART_LCR_DLAB) {
            uart->reg.dlm = value;
        } else {
            uart->reg.ier = value;
            uart_update_irq(uart);
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

bool uart_is_interrupted(riscv_uart *uart)
{
    bool ret = uart->is_interrupted;
    uart->is_interrupted = false;
    return ret;
}

void free_uart(__attribute__((unused)) riscv_uart *uart)
{
    /* Do nothing currently */
}
