#ifndef FIFO_H
#define FIFO_H

/* A minimalist FIFO buffer */

#define FIFO_LEN 64
#define FIFO_MASK (FIFO_LEN - 1)

struct fifo {
    uint8_t data[FIFO_LEN];
    unsigned int head, tail;
};

#define fifo_init(fifo) (fifo)->head = 0, (fifo)->tail = 0

#define fifo_is_empty(fifo) ((fifo)->head == (fifo)->tail)

#define fifo_is_full(fifo) ((fifo)->tail - (fifo)->head > FIFO_MASK)

#define fifo_put(fifo, value)                               \
    ({                                                      \
        unsigned int __ret = !fifo_is_full(fifo);           \
        if (__ret) {                                        \
            (fifo)->data[(fifo)->tail & FIFO_MASK] = value; \
            (fifo)->tail++;                                 \
        }                                                   \
        __ret;                                              \
    })

#define fifo_get(fifo, value)                               \
    ({                                                      \
        unsigned int __ret = !fifo_is_empty(fifo);          \
        if (__ret) {                                        \
            value = (fifo)->data[(fifo)->head & FIFO_MASK]; \
            (fifo)->head++;                                 \
        }                                                   \
        __ret;                                              \
    })

#endif
