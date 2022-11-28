#ifndef LOG_H
#define LOG_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

#ifdef DEBUG
#define LOG_DEBUG(format, ...) log_debug(format, ##__VA_ARGS__)
void log_debug(const char *format, ...);
#else
#define LOG_DEBUG(format, ...)
#endif

bool log_begin(void);
void log_end(void);
#endif /* LOG_H */
