//
// Created by Keuin on 2021/12/29.
//

#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "logging.h"
#include "netcheck.h"
#include "validate.h"

/**
 * Check network availability by testing a tcp communication.
 * @return Zero if success, non-zero if failed.
 */
int check_tcp(void *logger) {
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

/**
 * Check network availability by pinging a remote host.
 * Note that this routine requires an external ping program.
 * @return Zero if success, non-zero if failed.
 * @param logger the logger.
 * @param dest the destination host, whether a domain or an ip address.
 * @param ping path to the ping executable. If null, will use `/bin/ping`.
 * @return Zero if success, non-zero if failed.
 */
int check_ping(void *logger, const char *dest, const char *ping) {
    if (!is_valid_ipv4(dest)) {
        log_error(logger, "dest is not a valid IPv4 address.");
        return -1;
    }

#define BUFLEN 1024
#define RETURN(r) do { rv = (r); goto CP_RET; } while(0)
    int rv = 0;
    // Copied from https://stackoverflow.com/questions/8189935/is-there-any-way-to-ping-a-specific-ip-address-with-c
    if (ping == NULL) ping = "/bin/ping";
    int pipe_arr[2] = {-1};
    char buf[BUFLEN] = {0};

    // Create pipe - pipe_arr[0] is "reading end", pipe_arr[1] is "writing end"
    if (pipe(pipe_arr)) {
        perror("pipe()");
        log_error(logger, "pipe() failed.");
        return -1;
    }

    if (fork() == 0) {
        // child
        if (dup2(pipe_arr[1], STDOUT_FILENO) < 0) {
            log_error(logger, "dup2() failed.");
            exit(-1);
        }
        execl(ping, "ping", "-c 3", dest, (char *) NULL);
    } else {
        // parent
        if (wait(NULL) < 0) {
            perror("wait()");
            log_error(logger, "wait() failed.");
            RETURN(-1);
        }
        if (read(pipe_arr[0], buf, BUFLEN) < 0) {
            perror("read()");
            log_error(logger, "read() failed.");
            RETURN(-1);
        }
    }
    CP_RET:
    if (pipe_arr[0] >= 0) close(pipe_arr[0]);
    if (pipe_arr[1] >= 0) close(pipe_arr[1]);
    return (rv) ? (rv) : (strstr(buf, "time=") == NULL);
#undef BUFLEN
#undef RETURN
}