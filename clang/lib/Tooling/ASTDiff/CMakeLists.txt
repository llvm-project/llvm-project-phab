set(LLVM_LINK_COMPONENTS
  Support
  )

add_clang_library(clangToolingASTDiff
  ASTDiff.cpp
  ASTPatch.cpp
  LINK_LIBS
  clangBasic
  clangAST
  clangLex
  clangRewrite
  clangFrontend
  clangTooling
  clangToolingCore
  )
