#include "uart.h"
#include <signal.h>
#include <string.h>
#include <unistd.h>

#define uart_reg(uart, addr) uart->reg[addr - UART_BASE]

/* global variable to stop the infinite loop of thread, has data race */
volatile int thread_stop = 0;

static void thread(riscv_uart *uart)
{
    while (!thread_stop) {
        char c;
        scanf("%c", &c);
        pthread_mutex_lock(&uart->lock);

        while ((uart_reg(uart, UART_LSR) & UART_LSR_RX) == 1) {
            pthread_cond_wait(&uart->cond, &uart->lock);
        }
        uart_reg(uart, UART_RHR) = c;
        // not exactly used now
        uart->is_interrupt = 0;

        uart_reg(uart, UART_LSR) |= UART_LSR_RX;

        pthread_mutex_unlock(&uart->lock);
    }
}


bool init_uart(riscv_uart *uart)
{
    memset(uart->reg, 0, sizeof(UART_SIZE));
    uart->is_interrupt = false;
    // transmitter hold register is empty at first
    uart_reg(uart, UART_LSR) |= UART_LSR_TX;

    thread_stop = 0;
    pthread_mutex_init(&uart->lock, NULL);
    pthread_cond_init(&uart->cond, NULL);

    // create a thread for waiting input
    pthread_t tid;
    int ret = pthread_create(&tid, NULL, (void *) thread, (void *) uart);
    pthread_detach(tid);
    if (ret) {
        LOG_ERROR("Error %d when create thread!\n", ret);
        return false;
    }

    return true;
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

    pthread_mutex_lock(&uart->lock);

    uint64_t ret_value = -1;
    switch (addr) {
    case UART_RHR:
        ret_value = uart_reg(uart, addr);
        // no data in receive holding register after reading
        uart_reg(uart, UART_LSR) &= ~UART_LSR_RX;
        // thus the next input can come in
        pthread_cond_broadcast(&uart->cond);
        break;
    default:
        ret_value = uart_reg(uart, addr);
    }

    pthread_mutex_unlock(&uart->lock);
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

    pthread_mutex_lock(&uart->lock);

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
    pthread_mutex_unlock(&uart->lock);

    return true;
}

void free_uart(riscv_uart *uart)
{
    thread_stop = 1;

    pthread_mutex_destroy(&uart->lock);
    pthread_cond_destroy(&uart->cond);
}
