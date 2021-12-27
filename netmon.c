#include "logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

const char *logfile = "netmon.log";
int check_interval_seconds = 30; // seconds to sleep between checks
int max_check_failure = 5; // how many failures to reboot the system
int as_daemon = 0; // should run as a daemon process

volatile int require_reboot = 0;
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
    chdir("/");
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

/**
 * Check network availability.
 * @return Zero if success, non-zero if failed.
 */
int check_network() {
#define RETURN(r) do { rv = (r); goto RET; } while(0)
#define READ_SIZE 32

    int sock = -1, rv = 0;
    struct sockaddr_in serv_addr;
    const char *msg = "GET / HTTP/1.1\r\n"
                      "Host: www.gov.cn\r\n"
                      "\r\n";

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        log_error(logger, "socket() failed.");
        RETURN(-1);
    }

    const struct hostent *host = gethostbyname("www.gov.cn"); // TODO cache this

    if (!host) {
        herror("gethostbyname()");
        log_error(logger, "Cannot resolve test host.");
        RETURN(-1);
    }

    if (host->h_length <= 0) {
        log_error(logger, "Test host does not have at least one address.");
        RETURN(-1);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(80);
    serv_addr.sin_addr = **((struct in_addr **) host->h_addr_list);

    if (connect(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        log_error(logger, "connect() failed.");
        RETURN(-1);
    }

    ssize_t ss = 0; // send size
    while (ss < strlen(msg)) {
        ssize_t rd = send(sock, msg, strlen(msg), 0);
        if (rd < 0) {
            perror("send()");
            log_error(logger, "send() failed.");
            RETURN(-1);
        }
        ss += rd;
    }

    log_debug(logger, "HTTP request is sent. Reading response.");
    char response[READ_SIZE] = {0};
    ssize_t rs = 0; // receive size
    while (rs < READ_SIZE) {
        ssize_t rd = read(sock, response, READ_SIZE - rs);
        if (rd < 0) {
            perror("read()");
            log_error(logger, "read() failed.");
            RETURN(-1);
        } else {
            rs += rd;
        }
    }

    const char *expected = "HTTP/1.1 200 OK\r\n";
    if (memcmp(response, expected, strlen(expected)) != 0) {
        char buf[64];
        snprintf(buf, 63, "Unexpected response: %s", response);
        log_error(logger, buf);
        RETURN(-1);
    }
    RETURN(0);

    RET:
    if (sock >= 0) close(sock);
    return rv;
#undef RETURN
#undef READ_SIZE
}

void loop() {
    // check network and sleep
    // if fails too many times continuously, set require_reboot flag and return
    int failures = 0;
#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
    while (1) {
        log_info(logger, "Check network.");
        if (check_network() != 0) {
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
            require_reboot = 1;
            return;
        }
        log_info(logger, "Sleeping...");
        unsigned int t = check_interval_seconds;
        while ((t = sleep(t)));
    }
#pragma clang diagnostic pop
}

int main(int argc, char *argv[]) {
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "-t")) {
            check_interval_seconds = atoi(argv[i + 1]);
            ++i; // skip next value
        } else if (!strcmp(argv[i], "-n")) {
            max_check_failure = atoi(argv[i + 1]);
            ++i; // skip next value
        } else if (!strcmp(argv[i], "-l")) {
            logfile = argv[i + 1];
            ++i; // skip next value
        } else if (!strcmp(argv[i], "-d")) {
            as_daemon = 1;
        } else {
            int invalid = strcmp(argv[i], "-h") != 0 && strcmp(argv[i], "--help") != 0;
            if (invalid) printf("Unrecognized parameter \"%s\".\n", argv[i]);
            printf("Usage:\n"
                   "  %s [-t <check_interval>] [-n <max_failure>] [-l <log_file>] [-d]\n", argv[0]);
            return invalid != 0;
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
    if (require_reboot) {
        log_info(logger, "Trigger system reboot.");
        system("reboot");
    }
    log_free(logger);
    return 0;
}
