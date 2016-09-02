set(LLVM_LINK_COMPONENTS Support)

add_clang_library(clangRefactorRenameModule
  RenamingAction.cpp

  LINK_LIBS
  clangAST
  clangBasic
  clangFrontend
  clangToolingCore

  clangRefactorUSREngine
  )
