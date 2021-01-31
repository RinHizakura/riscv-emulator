#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>

#define LOG_ERROR(...) fprintf(stderr, __VA_ARGS__)

#ifdef DEBUG
#define LOG_DEBUG(...) fprintf(stderr, __VA_ARGS__)
#else
#define LOG_DEBUG(...)
#endif


#endif
