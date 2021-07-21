#ifndef MACROS
#define MACROS

#include <endian.h>

/* In our emulator, the memory is little endian, so we can just casting
 * memory to target pointer type if our host architecture is also the case.
 * If our architecture is big endian, then we should revise the order first
 *
 * Note:
 * I don't sure if the index of memory would overflow. If it will, the
 * implementation now will get error.
 */

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define read_len(bit, ptr, value) value = (*(uint##bit##_t *) (ptr))
#else
#define read_len(bit, ptr, value) \
    value = __builtin_bswap##bit(*(uint##bit##_t *) (ptr))
#endif

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define write_len(bit, ptr, value) \
    *(uint##bit##_t *) (ptr) = (uint##bit##_t)(value)
#else
#define write_len(bit, ptr, value) \
    *(uint##bit##_t *) (ptr) = __builtin_bswap##bit((uint##bit##_t)(value))
#endif

#endif
