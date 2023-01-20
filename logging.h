//
// Created by Keuin on 2021/12/28.
//

#ifndef NETMON_LOGGING_H
#define NETMON_LOGGING_H

#include <time.h>

#define die(args...) \
    do { \
        fprintf(stderr, args); \
        exit(1); \
    } while(0)

void *log_init(const char *filename);

void log_free(void *logger);

void log_print(void *logger, const char *level, time_t ts, const char *filename,
               int lineno, const char *msg);

#ifdef DEBUG
#define log_debug(logger, msg) log_print((logger), "DEBUG", time(NULL), __FILE__, __LINE__, (msg))
#else
#define log_debug(logger, msg) do { (logger); (msg); } while(0)
#endif
#define log_info(logger, msg) log_print((logger), "INFO", time(NULL), __FILE__, __LINE__, (msg))
#define log_warning(logger, msg) log_print((logger), "WARN", time(NULL), __FILE__, __LINE__, (msg))
#define log_error(logger, msg) log_print((logger), "ERROR", time(NULL), __FILE__, __LINE__, (msg))

#endif //NETMON_LOGGING_H
