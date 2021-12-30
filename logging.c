//
// Created by Keuin on 2021/12/28.
//

#include "logging.h"
#include "validate.h"
#include <stdio.h>


void *log_init(const char *filename) {
    FILE *fp = fopen(filename, "a");
    return fp;
}

void log_free(void *logger) {
    fflush(logger);
    fclose(logger);
}

void log_print(void *logger, const char *level, time_t ts, const char *filename, int lineno, const char *msg) {
    NOTNULL(logger);
    NOTNULL(level);
    NOTNULL(filename);
    NOTNULL(msg);
    char timestr[32];
    strftime(timestr, 31, "%Y-%m-%d %H:%M:%S", localtime(&ts));
    if (fprintf(logger, "[%s][%s][%s][%d] %s\n", timestr, level, filename, lineno, msg))
        fflush(logger);
    if (fprintf(stderr, "[%s][%s][%s][%d] %s\n", timestr, level, filename, lineno, msg))
        fflush(stderr);
}
