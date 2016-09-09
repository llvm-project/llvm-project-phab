set(LLVM_LINK_COMPONENTS
  Support
  )

add_clang_executable(clang-refactor
  Driver.cpp

  ModuleManager.cpp
  )

target_link_libraries(clang-refactor
  clangAST
  clangASTMatchers
  clangBasic
  clangFrontend
  clangTooling

  clangRefactorModuleCore
  )
