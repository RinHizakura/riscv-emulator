#include "uart.h"
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

#define uart_reg(uart, addr) uart->reg[addr - UART_BASE]


/* FIXME: Several pthread_* function is not completely doing
 * the error handling, we should take care of this. */

/* global variable to stop the infinite loop of thread, has data race */
volatile int thread_stop = 0;

static void thread(riscv_uart *uart)
{
    int infd = STDIN_FILENO;
    fd_set readfds;

    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    while (!__atomic_load_n(&thread_stop, __ATOMIC_SEQ_CST)) {
        FD_ZERO(&readfds);
        FD_SET(infd, &readfds);

        int result = select(infd + 1, &readfds, NULL, NULL, &timeout);

        // if error or timeout expired
        if (result <= 0)
            continue;

        assert(FD_ISSET(infd, &readfds));

        char c;
        int ret = read(infd, &c, 1);
        if (ret == -1)
            continue;

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

    if (pthread_mutex_init(&uart->lock, NULL))
        return false;

    if (pthread_cond_init(&uart->cond, NULL))
        return false;

    // create a thread for waiting input
    pthread_t tid;
    if (pthread_create(&tid, NULL, (void *) thread, (void *) uart)) {
        LOG_ERROR("Error when create thread!\n");
        return false;
    }

    uart->child_tid = tid;
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
    __atomic_store_n(&thread_stop, 1, __ATOMIC_SEQ_CST);
    pthread_join(uart->child_tid, NULL);
    pthread_mutex_destroy(&uart->lock);
    pthread_cond_destroy(&uart->cond);
}
