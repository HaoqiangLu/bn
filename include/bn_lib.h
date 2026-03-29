#ifndef _BN_LIB_H_
#define _BN_LIB_H_

#include "bn.h"

BigNum* bn_words_expend(BigNum *a, int words);
BigNum* bn_expand2(BigNum *a, int words);

void bn_correct_top(BigNum *a);

#endif /* _BN_LIB_H_ */