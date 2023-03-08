#ifndef RISCV_UART
#define RISCV_UART

/* Reference: http://byterunner.com/16550.html */

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

#include "exception.h"
#include "fifo.h"
#include "memmap.h"

// Receive holding register (read mode)
#define UART_RHR UART_BASE + 0
// Transmit holding register (write mode)
#define UART_THR UART_BASE + 0
// LSB of Divisor Latch when enabled
#define UART_DLL UART_BASE + 0

// Interrupt enable register
#define UART_IER UART_BASE + 1
// MSB of Divisor Latch when enabled
#define UART_DLM UART_BASE + 1

// Interrupt status register (read mode)
#define UART_ISR UART_BASE + 2
// FIFO control register (write mode)
#define UART_FCR UART_BASE + 2

// Line control register
#define UART_LCR UART_BASE + 3
// Modem control register
#define UART_MCR UART_BASE + 4
// Line status register
#define UART_LSR UART_BASE + 5
// Modem status register
#define UART_MSR UART_BASE + 6
// Scratchpad register
#define UART_SCR UART_BASE + 7


/* BIT 0:
 * 0 = no data in receive holding register or FIFO.
 * 1 = data has been receive and saved in the receive holding register or FIFO.
 */
#define UART_LSR_DR 0x1
/* BIT 5:
 * 0 = transmit holding register is full. 16550 will not accept any data for
 * transmission.
 * 1 = transmitter hold register (or FIFO) is empty. CPU can load
 * the next character.
 */
#define UART_LSR_THRE 0x20
/* BIT 6:
 * 0 = transmitter holding and shift registers are full.
 * 1 = transmit holding register is empty. In FIFO mode this bit is set to one
 * whenever the the transmitter FIFO and transmit shift register are empty. */
#define UART_LSR_TEMT 0x40

/* BIT 0:
 * 0 = disable the receiver ready interrupt.
 * 1 = enable the receiver ready interrupt.
 */
#define UART_IER_RDI 0x01
/* BIT 1:
 * 0 = disable the transmitter empty interrupt.
 * 1 = enable the transmitter empty interrupt.
 */
#define UART_IER_THRI 0x02

/* BIT 7 BAUD LATCH
 * 0 = disabled, normal operation
 * 1 = enabled
 */
#define UART_LCR_DLAB 0x80

/* BIT 0:
 * 0 = an interrupt is pending and the ISR contents may
 *     be used as a pointer to the appropriate interrupt service routine.
 * 1 = no interrupt penting.
 */
#define UART_ISR_NO_INT 0x01
#define UART_ISR_THRI 0x02
#define UART_ISR_RDI 0x04

typedef struct {
    struct {
        uint8_t dll;  // Divisor Latch Low
        uint8_t dlm;  // Divisor Latch High
        uint8_t isr;  // Interrupt Status
        uint8_t ier;  // Interrupt Enable
        uint8_t fcr;  // FIFO Control
        uint8_t lcr;  // Line Control
        uint8_t mcr;  // Modem Control
        uint8_t lsr;  // Line Status
        uint8_t msr;  // Modem Status
        uint8_t scr;  // Scratchpad
    } reg;

    bool is_interrupt;
    int infd;
    struct fifo rx_buf;
} riscv_uart;

bool init_uart(riscv_uart *uart);
uint64_t read_uart(riscv_uart *uart,
                   uint64_t addr,
                   uint8_t size,
                   riscv_exception *exc);
void tick_uart(riscv_uart *uart);
bool write_uart(riscv_uart *uart,
                uint64_t addr,
                uint8_t size,
                uint64_t value,
                riscv_exception *exc);
bool uart_is_interrupt(riscv_uart *uart);
void free_uart(riscv_uart *uart);

#endif
