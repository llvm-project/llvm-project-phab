set(LLVM_EXPORTED_SYMBOL_FILE ${CMAKE_CURRENT_SOURCE_DIR}/gold.exports)

if( LLVM_ENABLE_PIC AND LLVM_BINUTILS_INCDIR )
  include_directories( ${LLVM_BINUTILS_INCDIR} )

  set(LLVM_LINK_COMPONENTS
     ${LLVM_TARGETS_TO_BUILD}
     Linker
     LTO
     BitWriter
     IPO
     )

  add_llvm_loadable_module(LLVMgold
    gold-plugin.cpp
    )

if(WITH_POLLY AND LINK_POLLY_INTO_TOOLS)
  target_link_libraries(LLVMgold PUBLIC Polly)
endif(WITH_POLLY AND LINK_POLLY_INTO_TOOLS)

endif()
