#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "log.h"

#define MAX_LINE_LEN 256
#define MAX_RECORD_LINE_SHIFT 10
#define MAX_RECORD_LINE_MASK ((1 << MAX_RECORD_LINE_SHIFT) - 1)
#define MAX_RECORD_LINE (1 << MAX_RECORD_LINE_SHIFT)

/* A naive implementation to log the debug information in the ring buffer. */
struct logger {
    char line_buf[MAX_RECORD_LINE][MAX_LINE_LEN + 1];
    size_t line_cnt;
};
static struct logger *logger;

void __attribute__((constructor)) log_begin(void)
{
    logger = calloc(1, sizeof(struct logger));
}

void __attribute__((destructor)) log_end(void)
{
    FILE *trace_file = fopen("trace.out", "w");
    if (trace_file != NULL) {
        for (size_t i = 0; i < MAX_RECORD_LINE; i++) {
            size_t idx = (logger->line_cnt - i) & MAX_RECORD_LINE_MASK;
            if (logger->line_buf[idx][0] == '\0')
                break;
            fprintf(trace_file, "%s\n", logger->line_buf[idx]);
        }
        fclose(trace_file);
    }
    free(logger);
}

void log_debug(const char *format, ...)
{
    va_list vargs;
    char *buf = logger->line_buf[logger->line_cnt & MAX_RECORD_LINE_MASK];
    va_start(vargs, format);
    vsnprintf(buf, MAX_LINE_LEN, format, vargs);
    logger->line_cnt++;
    va_end(vargs);
}
