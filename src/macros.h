#ifndef MACROS
#define MACROS

#define read_len(bit, ptr, value)                            \
    do {                                                     \
        value = 0;                                           \
        for (int i = 0; i < ((bit) >> 3); i++)               \
            value |= ((uint64_t)(*((ptr) + i))) << (i << 3); \
    } while (0)

#define write_len(bit, ptr, value)                     \
    do {                                               \
        for (int i = 0; i < ((bit) >> 3); i++)         \
            *((ptr) + i) = (value >> (i << 3)) & 0xff; \
    } while (0)

#define container_of(ptr, type, member)               \
    ({                                                \
        void *__mptr = (void *) (ptr);                \
        ((type *) (__mptr - offsetof(type, member))); \
    })

#endif /* MACROS */
