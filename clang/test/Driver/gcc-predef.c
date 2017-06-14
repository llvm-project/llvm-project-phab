// Test that gcc preincludes stdc-predef.h on Linux
//
// RUN: %clang -c --target=i386-unknown-linux %s -Xclang -verify
// expected-no-diagnostics
#if defined( __GNUC__ )
  #if __GNUC__ > 0 && (__GNUC__ < 4 || __GNUC__ == 4 && __GNUC_MINOR__ < 8)
    /* check preinclude for gcc 4.8 and higher */
  #else
    #if !defined(  _STDC_PREDEF_H )
      #error "stdc-predef.h should be preincluded for GNU/Linux 4.8 and higher"
    #endif
  #endif
#endif
