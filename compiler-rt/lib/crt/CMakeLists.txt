add_compiler_rt_component(crt)

set(SHARED_CFLAGS
  -DSHARED
)

foreach(arch ${CRT_SUPPORTED_ARCH})
  add_compiler_rt_runtime(crtbegin
    OBJECT
    ARCHS ${arch}
    SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/crtbegin.c
    PARENT_TARGET crt)
  add_compiler_rt_runtime(crtbeginS
    OBJECT
    ARCHS ${arch}
    SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/crtbegin.c
    CFLAGS ${SHARED_CFLAGS}
    PARENT_TARGET crt)
  add_compiler_rt_runtime(crtend
    OBJECT
    ARCHS ${arch}
    SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/crtend.c
    PARENT_TARGET crt)
  add_compiler_rt_runtime(crtendS
    OBJECT
    ARCHS ${arch}
    SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/crtend.c
    CFLAGS ${SHARED_CFLAGS}
    PARENT_TARGET crt)
endforeach()
