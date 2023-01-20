#include "logging.h"
#include "netcheck.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>

#define OPTPARSE_IMPLEMENTATION
#define OPTPARSE_API static

#include "optparse.h"

const char *logfile = "netmon.log";
const char *pingdest = NULL; // which host to ping. If NULL, test tcp instead
const char *failcmd = "reboot"; // cmd to be executed. If NULL, reboot // TODO support blanks
unsigned int check_interval_seconds = 30; // seconds to sleep between checks
int max_check_failure = 5; // how many failures to reboot the system
int as_daemon = 0; // should run as a daemon process
unsigned int failure_sleep_seconds = 60; // seconds to sleep before resuming check after a network failure is detected
// TODO make failure_sleep_seconds configurable

volatile int failure_detected = 0;
void *logger = NULL;

void daemonize() {
    pid_t pid = 0;
    pid_t sid = 0;
    pid = fork();
    if (pid < 0) {
        perror("fork()");
        log_error(logger, "fork() failed.");
        exit(1);
    }
    if (pid > 0) {
        char buf[32];
        snprintf(buf, 31, "Child process: %d", pid);
        log_info(logger, buf);
        exit(0); // exit parent process
    }
    // unmask the file mode
    umask(0);
    // set new session
    sid = setsid();
    if (sid < 0) {
        perror("setsid()");
        log_error(logger, "setsid() failed.");
        exit(1);
    }
    if (chdir("/")) {
        perror("chdir()");
        log_error(logger, "chdir() failed,");
        exit(1);
    }
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

///**
// * Read and escape a string, then write to another buffer, with length limit.
// * @param si
// * @param so
// * @param maxlen
// */
//void escape(const char *si, char *so, size_t maxlen) {
//    size_t wb = 0; // bytes written
//    char c[4] = {'\0'};
//    while ((c[0] = *si++) != '\0') {
//        const char *s;
//        switch (c[0]) {
//            case '\r':
//                s = "\r";
//                break;
//            case '\n':
//                s = "\n";
//                break;
//            case '\t':
//                s = "\t";
//                break;
//            case '\v':
//                s = "\v";
//                break;
//            case '\b':
//                s = "\b";
//                break;
//            case '\a':
//                s = "\a";
//                break;
//            case '\f':
//                s = "\f";
//                break;
//            default:
//                s = c;
//                break;
//        }
//        strcpy(so, s)
//    }
//}

void loop() {
    // check network and sleep
    // if fails too many times continuously, set require_reboot flag and return
    int failures = 0;
#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
    while (1) {
        log_info(logger, "Check network.");
        if ((pingdest == NULL) ?
            (check_tcp(logger) != 0) :
            (check_ping(logger, pingdest, NULL) != 0)) {
            ++failures;
            char buf[64];
            snprintf(buf, 63, "Network failure detected. counter=%d", failures);
            log_info(logger, buf);
        } else {
            log_info(logger, "Network is OK.");
            failures = 0;
        }
        if (failures > max_check_failure) {
            log_info(logger, "Max failure times exceeded.");
            failure_detected = 1;
            failures = 0; // reset failure counter

            // handle a network failure event
            char tmp[256];
            snprintf(tmp, 255, "Run system command `%s`.", failcmd);
            log_info(logger, tmp);
            system(failcmd);

            snprintf(tmp, 255, "Wait %d secs before resume checking.\n", failure_sleep_seconds);
            log_debug(logger, tmp);
            unsigned t = failure_sleep_seconds;
            while ((t = sleep(t)));

            continue; // resume checking right now
        }
        log_info(logger, "Sleeping...");
        unsigned int t = check_interval_seconds;
        while ((t = sleep(t)));
    }
#pragma clang diagnostic pop
}

int main(int argc, char *argv[]) {
    struct optparse_long opts[] = {
            {"interval",    't', OPTPARSE_REQUIRED},
            {"max-failure", 'n', OPTPARSE_REQUIRED},
            {"log",         'l', OPTPARSE_REQUIRED},
            {"ping",        'p', OPTPARSE_REQUIRED},
            {"command",     'c', OPTPARSE_REQUIRED},
            {"daemon",      'd', OPTPARSE_NONE},
            {"help",        'h', OPTPARSE_NONE},
            {0}
    };
    int option;
    struct optparse options;

    char *end;

    optparse_init(&options, argv);
    while ((option = optparse_long(&options, opts, NULL)) != -1) {
        switch (option) {
            case 't':
                check_interval_seconds = (int) strtol(options.optarg, &end, 10);
                if (*end != '\0') {
                    die("Invalid check interval: %s\n", options.optarg);
                }
                if (check_interval_seconds <= 0) {
                    die("Check interval should be positive.\n");
                }
                break;
            case 'n':
                max_check_failure = (int) strtol(options.optarg, &end, 10);
                if (*end != '\0') {
                    die("Invalid max check failure times: %s\n",
                        options.optarg);
                }
                if (max_check_failure < 0) {
                    die("Max check failure times should not be negative.\n");
                }
                break;
            case 'l':
                logfile = strdup(options.optarg);
                break;
            case 'p':
                pingdest = strdup(options.optarg);
                break;
            case 'c':
                failcmd = strdup(options.optarg);
                break;
            case 'd':
                as_daemon = 1;
                break;
            case 'h':
                printf("Usage: %s "
                       "[-t <check_interval>] "
                       "[-n <max_failure>] "
                       "[-l <log_file>] "
                       "[-c <cmd>] "
                       "[-p <ping_host>] "
                       "[-d]\n",
                       argv[0]);
                exit(0);
            default:
                die("%s: %s\n", argv[0], options.errmsg);
        }
    }

    logger = log_init(logfile);
    log_debug(logger, "DEBUG logging is enabled.");
    if (as_daemon) {
        log_info(logger, "Daemonizing...");
        daemonize();
    }
    log_info(logger, "netmon is started.");
    loop();
    log_info(logger, "netmon is stopped.");
    log_free(logger);
    return 0;
}
