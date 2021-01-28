#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>

#define LOG_ERROR(...) fprintf(stderr, __VA_ARGS__)
#define asr_i64(value, amount) \
    (value < 0 ? ~(~value >> amount) : value >> amount)

#endif
