#ifdef DEBUG
#include <assert.h>
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
static struct logger *gLogger = NULL;

bool log_begin(void)
{
    assert(!gLogger);
    gLogger = calloc(1, sizeof(struct logger));
    return gLogger != NULL;
}

void log_end(void)
{
    assert(gLogger);

    FILE *trace_file = fopen("trace.out", "w");
    if (trace_file != NULL) {
        for (size_t i = 0; i < MAX_RECORD_LINE; i++) {
            size_t idx = (gLogger->line_cnt - i) & MAX_RECORD_LINE_MASK;
            if (gLogger->line_buf[idx][0] == '\0')
                break;
            fprintf(trace_file, "%s\n", gLogger->line_buf[idx]);
        }
        fclose(trace_file);
    }
    free(gLogger);
    gLogger = NULL;
}

void log_debug(const char *format, ...)
{
    assert(gLogger);

    va_list vargs;
    char *buf = gLogger->line_buf[gLogger->line_cnt & MAX_RECORD_LINE_MASK];
    va_start(vargs, format);
    vsnprintf(buf, MAX_LINE_LEN, format, vargs);
    va_end(vargs);

    gLogger->line_cnt++;
}
#else
bool log_begin(void)
{
    return true;
}
void log_end(void) {}
#endif /* DEBUG */
