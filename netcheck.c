//
// Created by Keuin on 2021/12/29.
//

#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "logging.h"
#include "netcheck.h"

/**
 * Check network availability.
 * @return Zero if success, non-zero if failed.
 */
int check_network(void *logger) {
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