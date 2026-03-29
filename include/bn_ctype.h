#ifndef _BN_CTYPE_H_
#define _BN_CTYPE_H_
#pragma once

#define BN_CTYPE_LOWER      (0x0001)
#define BN_CTYPE_UPPER      (0x0002)
#define BN_CTYPE_DIGIT      (0x0004)
#define BN_CTYPE_SPACE      (0x0008)
#define BN_CTYPE_XDIGIT     (0x0010)
#define BN_CTYPE_BLANK      (0x0020)
#define BN_CTYPE_CNTRL      (0x0040)
#define BN_CTYPE_GRAPH      (0x0080)
#define BN_CTYPE_PRINT      (0x0100)
#define BN_CTYPE_PUNCT      (0x0200)
#define BN_CTYPE_BASE64     (0x0400)
#define BN_CTYPE_ASN1PRINT  (0x0800)

#define BN_CTYPE_ALPHA (BN_CTYPE_LOWER | BN_CTYPE_UPPER)
#define BN_CTYPE_ALNUM (BN_CTYPE_ALPHA | BN_CTYPE_DIGIT)

#define BN_CTYPE_ASCII (~0)

#define BN_TOASCII(c)   ((int)(c))
#define BN_FROMASCII(c) ((int)(c))

int bn_ctype_check(int c, unsigned int mask);

int bn_is_digit(char c);
int bn_is_upper(char c);
int bn_is_lower(char c);
int bn_ascii_is_digit(char c);

int bn_to_lower(char c);
int bn_to_upper(char c);


#endif /* _BN_CTYPE_H_ */