set(LIBFUZZER_SOURCES
  FuzzerClangCounters.cpp
  FuzzerCrossOver.cpp
  FuzzerDriver.cpp
  FuzzerExtFunctionsDlsym.cpp
  FuzzerExtFunctionsDlsymWin.cpp
  FuzzerExtFunctionsWeak.cpp
  FuzzerExtraCounters.cpp
  FuzzerIO.cpp
  FuzzerIOPosix.cpp
  FuzzerIOWindows.cpp
  FuzzerLoop.cpp
  FuzzerMerge.cpp
  FuzzerMutate.cpp
  FuzzerSHA1.cpp
  FuzzerShmemPosix.cpp
  FuzzerShmemWindows.cpp
  FuzzerTracePC.cpp
  FuzzerUtil.cpp
  FuzzerUtilDarwin.cpp
  FuzzerUtilLinux.cpp
  FuzzerUtilPosix.cpp
  FuzzerUtilWindows.cpp
  )

CHECK_CXX_SOURCE_COMPILES("
  static thread_local int blah;
  int main() {
  return 0;
  }
  " HAS_THREAD_LOCAL)

set(LIBFUZZER_CFLAGS ${SANITIZER_COMMON_CFLAGS})

option(COMPILER_RT_ENABLE_LIBCXX "Use libc++ if available." OFF)

if (COMPILER_RT_ENABLE_LIBCXX)
  check_cxx_compiler_flag("-stdlib=libc++" COMPILER_RT_CXX_SUPPORTS_STDLIB)
  if(COMPILER_RT_CXX_SUPPORTS_STDLIB)
    list(APPEND LIBFUZZER_CFLAGS -stdlib=libc++)
    if(COMPILER_RT_HAS_LIBCXX_SOURCES)
      list(APPEND LIBFUZZER_CFLAGS -D_LIBCPP_ABI_VERSION=Fuzzer)
    endif()
  else()
    message(WARNING "Can't specify libc++ with '-stdlib='")
  endif()
endif()

if (CMAKE_CXX_FLAGS MATCHES "fsanitize-coverage")
  list(APPEND LIBFUZZER_CFLAGS -fno-sanitize-coverage=trace-pc-guard,edge,trace-cmp,indirect-calls,8bit-counters)
endif()

if(NOT HAS_THREAD_LOCAL)
  list(APPEND LIBFUZZER_CFLAGS -Dthread_local=__thread)
endif()

if(APPLE)
  set(FUZZER_SUPPORTED_OS osx)
endif()

add_compiler_rt_object_libraries(RTfuzzer
  OS ${FUZZER_SUPPORTED_OS}
  ARCHS ${FUZZER_SUPPORTED_ARCH}
  SOURCES ${LIBFUZZER_SOURCES}
  CFLAGS ${LIBFUZZER_CFLAGS})

add_compiler_rt_object_libraries(RTfuzzer_main
  OS ${FUZZER_SUPPORTED_OS}
  ARCHS ${FUZZER_SUPPORTED_ARCH}
  SOURCES FuzzerMain.cpp
  CFLAGS ${LIBFUZZER_CFLAGS})

add_compiler_rt_runtime(clang_rt.fuzzer
  STATIC
  OS ${FUZZER_SUPPORTED_OS}
  ARCHS ${FUZZER_SUPPORTED_ARCH}
  OBJECT_LIBS RTfuzzer RTfuzzer_main
  CFLAGS ${LIBFUZZER_CFLAGS}
  PARENT_TARGET fuzzer)

add_compiler_rt_runtime(clang_rt.fuzzer_no_main
  STATIC
  OS ${FUZZER_SUPPORTED_OS}
  ARCHS ${FUZZER_SUPPORTED_ARCH}
  OBJECT_LIBS RTfuzzer
  CFLAGS ${LIBFUZZER_CFLAGS}
  PARENT_TARGET fuzzer)

if(COMPILER_RT_HAS_LIBCXX_SOURCES AND
   COMPILER_RT_TEST_COMPILER_ID STREQUAL "Clang")
  foreach(arch ${FUZZER_SUPPORTED_ARCH})
    get_target_flags_for_arch(${arch} TARGET_CFLAGS)
    set(LIBCXX_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/libcxx_fuzzer_${arch})
    add_custom_libcxx(libcxx_fuzzer_${arch} ${LIBCXX_PREFIX}
      CFLAGS ${TARGET_CFLAGS} -D_LIBCPP_ABI_VERSION=Fuzzer)
    add_dependencies(clang_rt.fuzzer-${arch} libcxx_fuzzer_${arch})
    add_custom_command(TARGET clang_rt.fuzzer-${arch} POST_BUILD
      COMMAND
        ${PYTHON_EXECUTABLE} ${COMPILER_RT_LIBCXX_PATH}/utils/merge_archives.py
      ARGS
        -o $<TARGET_LINKER_FILE:clang_rt.fuzzer-${arch}>
        "$<TARGET_LINKER_FILE:clang_rt.fuzzer-${arch}>"
        "${CMAKE_CURRENT_BINARY_DIR}/libcxx_fuzzer_${arch}/lib/libc++.a"
    )
  endforeach()
endif()

if(COMPILER_RT_INCLUDE_TESTS)
  add_subdirectory(tests)
endif()
