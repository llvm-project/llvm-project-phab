set(LLVM_LINK_COMPONENTS
  Support
  )

add_clang_executable(clang-refactor
  Driver.cpp
  )

target_link_libraries(clang-refactor
  clangAST
  clangBasic
  clangFrontend
  clangTooling

  clangRefactorRenameModule

  clangRefactorUSREngine
  )
