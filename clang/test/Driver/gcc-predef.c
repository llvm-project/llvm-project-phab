// Test that gcc preincludes stdc-predef.h on Linux
//
// RUN: %clang -c %s --target=i686-unknown-gnu-linux \
// RUN: -Wno-macro-redefined -D__GNUC__=4 -D__GNUC_MINOR__=7 \
// RUN: --gcc-toolchain="" \
// RUN: --sysroot=%S/Inputs/basic_linux_tree -Xclang -verify 
// RUN: %clang -c -DFREESTANDING -ffreestanding %s \
// RUN: -Wno-macro-redefined -D__GNUC__=4 -D__GNUC_MINOR__=7 \
// RUN: --gcc-toolchain="" \
// RUN: --sysroot=%S/Inputs/basic_linux_tree -Xclang -verify
// RUN: %clang -c %s --target=i686-unknown-gnu-linux \
// RUN: -Wno-macro-redefined -D__GNUC__=4 -D__GNUC_MINOR__=9 \
// RUN: --gcc-toolchain="" \
// RUN: --sysroot=%S/Inputs/basic_linux_tree -Xclang -verify 
// RUN: %clang -c -DFREESTANDING -ffreestanding %s \
// RUN: -Wno-macro-redefined -D__GNUC__=4 -D__GNUC_MINOR__=9 \
// RUN: --gcc-toolchain="" \
// RUN: --sysroot=%S/Inputs/basic_linux_tree -Xclang -verify
// expected-no-diagnostics

// Test 1: not freestanding
#ifndef FREESTANDING
#if defined( __GNUC__ )
  #if __GNUC__ > 0 && (__GNUC__ < 4 || __GNUC__ == 4 && __GNUC_MINOR__ < 8)
    /* Preinclude <stdc-predef.h> only for gcc 4.8 and higher */
    #if defined( DUMMY_STDC_PREDEF )
      #error "stdc-predef.h should not be preincluded for gcc < 4.8.x"
    #endif
  #else
    #if !defined( DUMMY_STDC_PREDEF )
      #error "stdc-predef.h should be preincluded for GNU/Linux 4.8 and higher"
    #endif
  #endif
#endif

// Test 2: freestanding
#else
  // Verify -ffreestanding option does not pre-include <stdc-predef.h>.
  #if defined( DUMMY_STDC_PREDEF )
    #error "stdc-predef.h should not be preincluded when -ffreestanding"
  #endif
#endif
