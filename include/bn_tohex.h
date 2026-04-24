#ifndef _BN_INTERNAL_TO_HEX_H_
#define _BN_INTERNAL_TO_HEX_H_
#pragma once

static bn_inline size_t to_hex(char *buf, uint8_t n, const char hexdig[17])
{
    *buf++ = hexdig[(n >> 4) & 0xf];
    *buf = hexdig[n & 0xf];
    return 2;
}

static bn_inline size_t bn_to_lower_hex(char *buf, uint8_t n)
{
    static const char hexdig[17] = "0123456789abcdef";
    return to_hex(buf, n, hexdig);
}

bn_inline size_t bn_to_hex(char *buf, uint8_t n)
{
    static const char hexdig[17] = "0123456789ABCDEF";
    return to_hex(buf, n, hexdig);
}

#endif