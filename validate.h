//
// Created by Keuin on 2021/12/30.
//

#ifndef NETMON_VALIDATE_H
#define NETMON_VALIDATE_H

#include <stdlib.h>

int is_valid_ipv4(const char *s);

#define NOTNULL(ptr) do { \
                          if ((ptr) == NULL) { \
                           fprintf(stderr, "NotNull check failed: "#ptr " is null. (" __FILE__ ":%d)\n", __LINE__); \
                           abort(); \
                     } } while(0)

#endif //NETMON_VALIDATE_H
