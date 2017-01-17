add_compiler_rt_component(crt)

function(add_compiler_rt_crt_file name)
  cmake_parse_arguments(CRT
    ""
    "SOURCE"
    "CFLAGS"
    ${ARGN})
  add_custom_command(
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${name}.o
      COMMAND ${CMAKE_C_COMPILER} ${CMAKE_CURRENT_SOURCE_DIR}/${CRT_SOURCE} ${CMAKE_C_CFLAGS} ${CRT_CFLAGS} -c -o ${COMPILER_RT_LIBRARY_OUTPUT_DIR}/${name}.o
      DEPENDS ${CRT_SOURCE}
      COMMENT "Building C object ${name}.o")
  add_custom_target(${name} DEPENDS ${name}.o)
  add_custom_target(install-${name}
                    DEPENDS ${name}
                    COMMAND "${CMAKE_COMMAND}"
                          -DCMAKE_INSTALL_COMPONENT=${name}
                          -P "${CMAKE_BINARY_DIR}/cmake_install.cmake")
endfunction()

add_compiler_rt_crt_file(crtbegin
  SOURCE crtbegin.c)
add_compiler_rt_crt_file(crtbeginS
  SOURCE crtbegin.c
  CFLAGS -DSHARED)

add_compiler_rt_crt_file(crtend
  SOURCE crtend.c)
add_compiler_rt_crt_file(crtendS
  SOURCE crtend.c
  CFLAGS -DSHARED)

add_dependencies(crt crtbegin crtbeginS crtend crtendS)
