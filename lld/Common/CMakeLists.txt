if(NOT LLD_BUILT_STANDALONE)
  set(tablegen_deps intrinsics_gen)
endif()

if(BUILD_SHARED_LIBS)
  set(shared_link_libs ${LLVM_PTHREAD_LIB})
endif(BUILD_SHARED_LIBS)

add_lld_library(lldCommon
  Reproduce.cpp
  TargetOptionsCommandFlags.cpp
  Threads.cpp
  Version.cpp

  ADDITIONAL_HEADER_DIRS
  ${LLD_INCLUDE_DIR}/lld/Common

  LINK_COMPONENTS
  Codegen
  MC
  Option
  Support
  Target

  LINK_LIBS
  ${shared_link_libs}

  DEPENDS
  ${tablegen_deps}
  )
