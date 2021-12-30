//
// Created by Keuin on 2021/12/29.
//

#ifndef NETMON_NETCHECK_H
#define NETMON_NETCHECK_H


int check_tcp(void *logger);

int check_ping(void *logger, const char *dest, const char *ping);

#endif //NETMON_NETCHECK_H
