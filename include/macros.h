#ifndef _BN_MACROS_H_
#define _BN_MACROS_H_

/*
 * 根据配置自定义 __FILE__ 和 __LINE__ 宏
 */
#ifndef BN_FILE
 #ifdef BN_NO_FILENAMES
  #define BN_FILE ""
  #define BN_LINE 0
 #else
  #define BN_FILE __FILE__
  #define BN_LINE __LINE__
 #endif
#endif

/*
 * __func__ 在C99中标准化，因此对声称支持C99及以上的编译器，可安全使用；
 * GNU C从2.x版本开始提供__FUNCTION__（早于C99），但仅当__STDC_VERSION__存在时使用（符合ISO C标准）；
 * 若以上都不适用，检查是否是MSVC，使用__FUNCTION__；
 * 若所有情况都不满足，使用静态字符串"(unknown function)"。
 */
#ifndef BN_FUNC
 #if defined(__STDC_VERSION__)
  #if __STDC_VERSION__ >= 199901L
   #define BN_FUNC __func__
  #elif defined(__GNUC__) && __GUNC__ >= 2
   #define BN_FUNC __FUNCTION__
  #endif
 #elif defined(_MSC_VER_)
  #define BN_FUNC __FUNCTION__
 #endif

 #ifndef BN_FUNC
  #define BN_FUNC "(unknown function)"
 #endif
#endif

#endif /* _BN_MACROS_H_ */