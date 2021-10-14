#include "plic.h"
#include "irq.h"

uint64_t read_plic(riscv_plic *plic,
                   uint64_t addr,
                   uint8_t size,
                   riscv_exception *exc)
{
    // support only word-aligned and word size access now
    if ((size != 32) || (addr & 0x3))
        goto read_plic_fail;

    if (addr >= PLIC_PRIORITY && addr < PLIC_PRIORITY_END) {
        return plic->priority[(addr - PLIC_PRIORITY) / 4];
    }

    else if (addr >= PLIC_PENDING && addr < PLIC_PENDING_END) {
        return plic->pending[(addr - PLIC_PENDING) / 4];
    }

    else if (addr >= PLIC_ENABLE && addr < PLIC_ENABLE_END) {
        return plic->enable[(addr - PLIC_ENABLE) / 4];
    } else {
        switch (addr) {
        case PLIC_THRESHOLD_0:
            return plic->threshold[0];
        case PLIC_CLAIM_0:
            return plic->claim[0];
        case PLIC_THRESHOLD_1:
            return plic->threshold[1];
        case PLIC_CLAIM_1:
            return plic->claim[1];
        default:
            goto read_plic_fail;
        }
    }

read_plic_fail:
    LOG_ERROR("read plic addr %lx for size %d failed\n", addr, size);
    exc->exception = LoadAccessFault;
    return -1;
}


/* TODO: some registers allow double-word access to run linux,
 * but they required more check for correctness */
bool write_plic(riscv_plic *plic,
                uint64_t addr,
                uint8_t size,
                uint64_t value,
                riscv_exception *exc)
{
    if (((size != 32) && (size != 64)) || (addr & 0x3))
        goto write_plic_fail;

    uint32_t lo = value & 0xffffffff;
    uint32_t hi = (value >> 32);

    if (addr >= PLIC_PRIORITY && addr < PLIC_PRIORITY_END) {
        plic->priority[(addr - PLIC_PRIORITY) / 4] = lo;
        if (size == 64)
            plic->priority[((addr - PLIC_PRIORITY) / 4) + 1] = hi;
    }

    else if (addr >= PLIC_PENDING && addr < PLIC_PENDING_END) {
        plic->pending[(addr - PLIC_PENDING) / 4] = lo;
        if (size == 64)
            plic->pending[((addr - PLIC_PENDING) / 4) + 1] = hi;
    }

    else if (addr >= PLIC_ENABLE && addr < PLIC_ENABLE_END) {
        plic->enable[(addr - PLIC_ENABLE) / 4] = lo;
        if (size == 64)
            plic->enable[((addr - PLIC_ENABLE) / 4) + 1] = hi;
    } else {
        if (size == 64)
            goto write_plic_fail;

        switch (addr) {
        case PLIC_THRESHOLD_0:
            plic->threshold[0] = lo;
            break;
        case PLIC_CLAIM_0:
            plic->pending[value >> 5] =
                plic->pending[value >> 5] & ~(1 << (value & 0x1f));
            plic->claim[0] = 0;
            break;
        case PLIC_THRESHOLD_1:
            plic->threshold[1] = lo;
            break;
        case PLIC_CLAIM_1:
            plic->pending[value >> 5] =
                plic->pending[value >> 5] & ~(1 << (value & 0x1f));
            plic->update_irq = true;
            plic->claim[1] = 0;
            break;
        default:
            goto write_plic_fail;
        }
    }
    return true;

write_plic_fail:
    LOG_ERROR("wrtie plic addr %lx for size %d failed\n", addr, size);
    exc->exception = StoreAMOAccessFault;
    return false;
}

static void update_pending(riscv_plic *plic, uint32_t irq)
{
    /* FIXME: check the enable bit of plic register first */
    uint64_t idx = irq >> 5;
    plic->pending[idx] = plic->pending[idx] | (1 << (irq & 0x1f));
    plic->update_irq = true;
}

static bool update_irq(riscv_plic *plic)
{
    /* FIXME:
     *  1. How do we know we should perform an interrupt claim on
     *     context 1 (S-mode)?
     *  2. The priority of interrput should be consider */

    uint32_t pending = (plic->pending[UART0_IRQ >> 5] >> UART0_IRQ) & 1;
    uint32_t enable = (plic->enable[32] >> UART0_IRQ) & 1;
    if (pending && enable) {
        plic->claim[1] = UART0_IRQ;
        return true;
    }

    pending = (plic->pending[VIRTIO_IRQ >> 5] >> VIRTIO_IRQ) & 1;
    enable = (plic->enable[32] >> VIRTIO_IRQ) & 1;
    if (pending && enable) {
        plic->claim[1] = VIRTIO_IRQ;
        return true;
    }

    plic->claim[1] = 0;
    return false;
}

void tick_plic(riscv_plic *plic,
               riscv_csr *csr,
               bool is_uart_irq,
               bool is_virtio_irq)
{
    if (is_uart_irq)
        update_pending(plic, UART0_IRQ);
    if (is_virtio_irq)
        update_pending(plic, VIRTIO_IRQ);

    if (plic->update_irq) {
        // pending the external interrput bit
        if (update_irq(plic))
            set_csr_bits(csr, MIP, MIP_SEIP);
        plic->update_irq = false;
    }
}
