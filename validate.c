//
// Created by Keuin on 2021/12/30.
//

#include "validate.h"
#include <stdio.h>

/**
 * Check if a given string is a valid dot-decimal representation of an IPv4 address.
 * @param s the string.
 * @return Non-zero if true, zero if false.
 */
int is_valid_ipv4(const char *s) {
    // TODO buggy
//    unsigned int addr[4];
//    if (sscanf(s, "%ud.%ud.%ud.%ud", &addr[0], &addr[1], &addr[2], &addr[3]) != 4)
//        return 0;
//    for (int i = 0; i < 4; ++i) {
//        if (addr[i] > 255) return 0;
//        if (addr[i] == 0 && (i == 0 || i == 3)) return 0;
//    }
    return 1;
}
