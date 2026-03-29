#ifndef _BN_CONFIGURATION_H_
#define _BN_CONFIGURATION_H_
#pragma once

#if defined(__WORDSIZE)
    #define BN_PLATFORM_WORDSIZE __WORDSIZE
#elif defined(_WIN64) || defined(__64BIT__) || \
      defined(__LP64__) || defined(_LP64) || \
      defined(__x86_64__) || defined(__amd64__) || defined(_M_X64) || defined(_AMD64_) || \
      defined(__aarch64__) || defined(__arm64__) || defined(_M_ARM64) || \
      defined(__ppc64__) || defined(__powerpc64__) || \
      defined(__riscv64) || defined(__mips64)
    #define BN_PLATFORM_WORDSIZE 64
#else
    #define BN_PLATFORM_WORDSIZE 32
#endif

#endif /* _BN_CONFIGURATION_H_ */