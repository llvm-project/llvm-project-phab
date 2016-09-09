add_clang_library(clangRefactorModuleCore
  RefactoringModule.cpp

  LINK_LIBS
  clangAST
  clangASTMatchers
  clangBasic
  clangFrontend
  clangLex
  clangTooling
  clangToolingCore
  )
