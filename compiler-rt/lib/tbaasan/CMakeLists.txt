include_directories(..)

# Runtime library sources and build flags.
set(TBAASAN_RTL_SOURCES
  tbaasan.cc
  tbaasan_interceptors.cc)
set(TBAASAN_COMMON_CFLAGS ${SANITIZER_COMMON_CFLAGS})
append_rtti_flag(OFF TBAASAN_COMMON_CFLAGS)
# Prevent clang from generating libc calls.
append_list_if(COMPILER_RT_HAS_FFREESTANDING_FLAG -ffreestanding TBAASAN_COMMON_CFLAGS)

# Static runtime library.
add_compiler_rt_component(tbaasan)

foreach(arch ${TBAASAN_SUPPORTED_ARCH})
  set(TBAASAN_CFLAGS ${TBAASAN_COMMON_CFLAGS})
  append_list_if(COMPILER_RT_HAS_FPIE_FLAG -fPIE TBAASAN_CFLAGS)
  add_compiler_rt_runtime(clang_rt.tbaasan
    STATIC
    ARCHS ${arch}
    SOURCES ${TBAASAN_RTL_SOURCES}
            $<TARGET_OBJECTS:RTInterception.${arch}>
            $<TARGET_OBJECTS:RTSanitizerCommon.${arch}>
            $<TARGET_OBJECTS:RTSanitizerCommonLibc.${arch}>
    CFLAGS ${TBAASAN_CFLAGS}
    PARENT_TARGET tbaasan)
  add_sanitizer_rt_symbols(clang_rt.tbaasan
    ARCHS ${arch}
    EXTRA tbaasan.syms.extra)
  add_dependencies(tbaasan
    clang_rt.tbaasan-${arch}-symbols)
endforeach()

