//===- ASTDiff.h - AST differencing API -----------------------*- C++ -*- -===//
//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file specifies an interface that can be used to compare C++ syntax
// trees.
//
// We use the gumtree algorithm which combines a heuristic top-down search that
// is able to match large subtrees that are equivalent, with an optimal
// algorithm to match small subtrees.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLING_ASTDIFF_ASTDIFF_H
#define LLVM_CLANG_TOOLING_ASTDIFF_ASTDIFF_H

#include "clang/Frontend/ASTUnit.h"
#include "clang/Tooling/ASTDiff/ASTDiffInternal.h"

namespace clang {
namespace diff {

enum ChangeKind {
  NoChange,
  Delete,    // (Src): delete node Src.
  Update,    // (Src, Dst): update the value of node Src to match Dst.
  Insert,    // (Src, Dst, Pos): insert Src as child of Dst at offset Pos.
  Move,      // (Src, Dst, Pos): move Src to be a child of Dst at offset Pos.
  UpdateMove // Same as Move plus Update.
};

using NodeRef = const Node &;

struct ComparisonOptions {
  /// During top-down matching, only consider nodes of at least this height.
  int MinHeight = 2;

  /// During bottom-up matching, match only nodes with at least this value as
  /// the ratio of their common descendants.
  double MinSimilarity = 0.5;

  /// Whenever two subtrees are matched in the bottom-up phase, the optimal
  /// mapping is computed, unless the size of either subtrees exceeds this.
  int MaxSize = 100;

  bool StopAfterTopDown = false;
  bool StopAfterBottomUp = false;

  /// Returns false if the nodes should never be matched.
  bool isMatchingAllowed(NodeRef N1, NodeRef N2) const;
};

class ASTDiff {
public:
  ASTDiff(SyntaxTree &Src, SyntaxTree &Dst, const ComparisonOptions &Options);
  ~ASTDiff();

  const Node *getMapped(NodeRef N) const;

  class Impl;

private:
  std::unique_ptr<Impl> DiffImpl;
};

/// SyntaxTree objects represent subtrees of the AST.
/// They can be constructed from any Decl or Stmt.
class SyntaxTree {
public:
  /// Constructs a tree from a translation unit.
  SyntaxTree(ASTUnit &AST);
  /// Constructs a tree from any AST node.
  template <class T>
  SyntaxTree(T *Node, ASTUnit &AST)
      : TreeImpl(llvm::make_unique<Impl>(this, Node, AST)) {}
  SyntaxTree(SyntaxTree &&Other) = default;
  SyntaxTree &operator=(SyntaxTree &&Other) = default;
  ~SyntaxTree();

  ASTUnit &getASTUnit() const;
  const ASTContext &getASTContext() const;
  SourceManager &getSourceManager() const;
  const LangOptions &getLangOpts() const;
  StringRef getFilename() const;

  int getSize() const;
  NodeRef getRoot() const;
  NodeId getRootId() const;
  using PreorderIterator = const Node *;
  PreorderIterator begin() const;
  PreorderIterator end() const;

  NodeRef getNode(NodeId Id) const;

  /// Serialize the node attributes to a string representation. This should
  /// uniquely distinguish nodes of the same kind. Note that this function
  /// just
  /// returns a representation of the node value, not considering descendants.
  std::string getNodeValue(NodeRef Node) const;

  class Impl;
  std::unique_ptr<Impl> TreeImpl;
};

/// Represents a Clang AST node, alongside some additional information.
struct Node {
  SyntaxTree::Impl &Tree;
  NodeId Parent, LeftMostDescendant, RightMostDescendant;
  int Depth, Height, Shift = 0;
  ast_type_traits::DynTypedNode ASTNode;
  SmallVector<NodeId, 4> Children;
  ChangeKind Change = NoChange;
  Node(SyntaxTree::Impl &Tree) : Tree(Tree), Children() {}
  Node(NodeRef Other) = delete;
  explicit Node(Node &&Other) = default;
  Node &operator=(NodeRef Other) = delete;
  Node &operator=(Node &&Other) = default;

  NodeId getId() const;
  SyntaxTree &getTree() const;
  const Node *getParent() const;
  NodeRef getChild(size_t Index) const;
  size_t getNumChildren() const { return Children.size(); }
  ast_type_traits::ASTNodeKind getType() const;
  StringRef getTypeLabel() const;
  bool isLeaf() const { return Children.empty(); }
  bool isMacro() const;
  llvm::Optional<StringRef> getIdentifier() const;
  llvm::Optional<std::string> getQualifiedIdentifier() const;

  NodeRefIterator begin() const;
  NodeRefIterator end() const;

  int findPositionInParent() const;

  // Returns the starting and ending offset of the node in its source file.
  std::pair<unsigned, unsigned> getSourceRangeOffsets() const;
};

struct NodeRefIterator {
  SyntaxTree::Impl *Tree;
  const NodeId *IdPointer;
  NodeRefIterator(SyntaxTree::Impl *Tree, const NodeId *IdPointer)
      : Tree(Tree), IdPointer(IdPointer) {}
  NodeRef operator*() const;
  NodeRefIterator &operator++();
  NodeRefIterator &operator+(int Offset);
  bool operator!=(const NodeRefIterator &Other) const;
};

} // end namespace diff
} // end namespace clang

#endif
