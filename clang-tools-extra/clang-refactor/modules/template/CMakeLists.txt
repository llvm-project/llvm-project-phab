set(LLVM_LINK_COMPONENTS support)

add_clang_library(clangRefactorTemplateModule
  TemplateModule.cpp

  LINK_LIBS
  clangAST
  clangASTMatchers
  clangBasic
  clangFrontend
  clangTooling
  )
