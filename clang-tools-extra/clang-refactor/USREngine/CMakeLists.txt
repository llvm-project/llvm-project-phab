set(LLVM_LINK_COMPONENTS Support)

add_clang_library(clangRefactorUSREngine
  USRFinder.cpp
  USRFindingAction.cpp
  USRLocFinder.cpp

  LINK_LIBS
  clangAST
  clangASTMatchers
  clangBasic
  clangIndex
  clangLex
  )
