#ifndef LOG_H
#define LOG_H

#include <stdarg.h>
#include <stdio.h>

#define LOG_ERROR(...) fprintf(stderr, __VA_ARGS__)

#ifdef DEBUG
#define LOG_DEBUG(format, ...) log_debug(format, ##__VA_ARGS__)
#else
#define LOG_DEBUG(format, ...)
#endif

void log_debug(const char *format, ...);
#endif
