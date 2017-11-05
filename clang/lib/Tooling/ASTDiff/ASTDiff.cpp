//===- ASTDiff.cpp - AST differencing implementation-----------*- C++ -*- -===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains definitons for the AST differencing interface.
//
//===----------------------------------------------------------------------===//

#include "clang/Tooling/ASTDiff/ASTDiff.h"

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Lex/Lexer.h"
#include "llvm/ADT/PriorityQueue.h"

#include <limits>
#include <memory>
#include <unordered_set>

using namespace llvm;
using namespace clang;

namespace clang {
namespace diff {

namespace {
/// Maps nodes of the left tree to ones on the right, and vice versa.
class Mapping {
public:
  Mapping() = default;
  Mapping(Mapping &&Other) = default;
  Mapping &operator=(Mapping &&Other) = default;

  Mapping(size_t Size) {
    SrcToDst = llvm::make_unique<NodeId[]>(Size);
    DstToSrc = llvm::make_unique<NodeId[]>(Size);
  }

  void link(NodeId Src, NodeId Dst) {
    SrcToDst[Src] = Dst, DstToSrc[Dst] = Src;
  }

  NodeId getDst(NodeId Src) const { return SrcToDst[Src]; }
  NodeId getSrc(NodeId Dst) const { return DstToSrc[Dst]; }
  bool hasSrc(NodeId Src) const { return getDst(Src).isValid(); }
  bool hasDst(NodeId Dst) const { return getSrc(Dst).isValid(); }

private:
  std::unique_ptr<NodeId[]> SrcToDst, DstToSrc;
};
} // end anonymous namespace

class ASTDiff::Impl {
public:
  SyntaxTree::Impl &T1, &T2;
  Mapping TheMapping;

  Impl(SyntaxTree::Impl &T1, SyntaxTree::Impl &T2,
       const ComparisonOptions &Options);

  /// Matches nodes one-by-one based on their similarity.
  void computeMapping();

  // Compute Change for each node based on similarity.
  void computeChangeKinds(Mapping &M);

  const Node *getMapped(const std::unique_ptr<SyntaxTree::Impl> &Tree,
                        NodeRef N) const;

private:
  // Returns true if the two subtrees are identical.
  bool identical(NodeRef N1, NodeRef N2) const;

  // Returns false if the nodes must not be mached.
  bool isMatchingPossible(NodeRef N1, NodeRef N2) const;

  // Returns true if the nodes' parents are matched.
  bool haveSameParents(const Mapping &M, NodeRef N1, NodeRef N2) const;

  // Uses an optimal albeit slow algorithm to compute a mapping between two
  // subtrees, but only if both have fewer nodes than MaxSize.
  void addOptimalMapping(Mapping &M, NodeRef N1, NodeRef N2) const;

  // Computes the ratio of common descendants between the two nodes.
  // Descendants are only considered to be equal when they are mapped.
  double getJaccardSimilarity(const Mapping &M, NodeRef N1, NodeRef N2) const;

  // Returns the node that has the highest degree of similarity.
  const Node *findCandidate(const Mapping &M, NodeRef N1) const;

  // Returns a mapping of identical subtrees.
  Mapping matchTopDown();

  // Tries to match any yet unmapped nodes, in a bottom-up fashion.
  void matchBottomUp(Mapping &M) const;

  const ComparisonOptions &Options;

  friend class ZhangShashaMatcher;
};

namespace {
struct NodeList {
  SyntaxTree::Impl &Tree;
  std::vector<NodeId> Ids;
  NodeList(SyntaxTree::Impl &Tree) : Tree(Tree) {}
  void push_back(NodeId Id) { Ids.push_back(Id); }
  NodeRefIterator begin() const { return {&Tree, &*Ids.begin()}; }
  NodeRefIterator end() const { return {&Tree, &*Ids.end()}; }
  NodeRef operator[](size_t Index) { return *(begin() + Index); }
  size_t size() { return Ids.size(); }
  void sort() { std::sort(Ids.begin(), Ids.end()); }
};
} // end anonymous namespace

/// Represents the AST of a TranslationUnit.
class SyntaxTree::Impl {
public:
  Impl(SyntaxTree *Parent, ASTContext &AST);
  /// Constructs a tree from an AST node.
  Impl(SyntaxTree *Parent, Decl *N, ASTContext &AST);
  Impl(SyntaxTree *Parent, Stmt *N, ASTContext &AST);
  template <class T>
  Impl(SyntaxTree *Parent,
       typename std::enable_if<std::is_base_of<Stmt, T>::value, T>::type *Node,
       ASTContext &AST)
      : Impl(Parent, dyn_cast<Stmt>(Node), AST) {}
  template <class T>
  Impl(SyntaxTree *Parent,
       typename std::enable_if<std::is_base_of<Decl, T>::value, T>::type *Node,
       ASTContext &AST)
      : Impl(Parent, dyn_cast<Decl>(Node), AST) {}

  SyntaxTree *Parent;
  ASTContext &AST;
  PrintingPolicy TypePP;
  /// Nodes in preorder.
  std::vector<Node> Nodes;
  NodeList Leaves;
  // Maps preorder indices to postorder ones.
  std::vector<int> PostorderIds;
  NodeList NodesBfs;

  int getSize() const { return Nodes.size(); }
  NodeRef getRoot() const { return getNode(getRootId()); }
  NodeId getRootId() const { return 0; }
  PreorderIterator begin() const { return &getRoot(); }
  PreorderIterator end() const { return begin() + getSize(); }

  NodeRef getNode(NodeId Id) const { return Nodes[Id]; }
  Node &getMutableNode(NodeId Id) { return Nodes[Id]; }
  Node &getMutableNode(NodeRef N) { return getMutableNode(N.getId()); }
  int getNumberOfDescendants(NodeRef N) const;
  bool isInSubtree(NodeRef N, NodeRef SubtreeRoot) const;
  int findPositionInParent(NodeRef Id, bool Shifted = false) const;

  std::string getRelativeName(const NamedDecl *ND,
                              const DeclContext *Context) const;
  std::string getRelativeName(const NamedDecl *ND) const;

  std::string getNodeValue(NodeRef Node) const;
  std::string getDeclValue(const Decl *D) const;
  std::string getStmtValue(const Stmt *S) const;

private:
  void initTree();
  void setLeftMostDescendants();
};

NodeRef NodeRefIterator::operator*() const { return Tree->getNode(*IdPointer); }

NodeRefIterator &NodeRefIterator::operator++() { return ++IdPointer, *this; }
NodeRefIterator &NodeRefIterator::operator+(int Offset) {
  return IdPointer += Offset, *this;
}

bool NodeRefIterator::operator!=(const NodeRefIterator &Other) const {
  assert(Tree == Other.Tree &&
         "Cannot compare two iterators of different trees.");
  return IdPointer != Other.IdPointer;
}

static bool isSpecializedNodeExcluded(const Decl *D) { return D->isImplicit(); }
static bool isSpecializedNodeExcluded(const Stmt *S) { return false; }
static bool isSpecializedNodeExcluded(CXXCtorInitializer *I) {
  return !I->isWritten();
}

template <class T> static bool isNodeExcluded(const SourceManager &SM, T *N) {
  if (!N)
    return true;
  SourceLocation SLoc = N->getSourceRange().getBegin();
  if (SLoc.isValid()) {
    // Ignore everything from other files.
    if (!SM.isInMainFile(SLoc))
      return true;
    // Ignore macros.
    if (SLoc != SM.getSpellingLoc(SLoc))
      return true;
  }
  return isSpecializedNodeExcluded(N);
}

namespace {
// Sets Height, Parent and Children for each node.
struct PreorderVisitor : public RecursiveASTVisitor<PreorderVisitor> {
  int Id = 0, Depth = 0;
  NodeId Parent;
  SyntaxTree::Impl &Tree;

  PreorderVisitor(SyntaxTree::Impl &Tree) : Tree(Tree) {}

  template <class T> std::tuple<NodeId, NodeId> PreTraverse(T *ASTNode) {
    NodeId MyId = Id;
    Tree.Nodes.emplace_back(Tree);
    Node &N = Tree.getMutableNode(MyId);
    N.Parent = Parent;
    N.Depth = Depth;
    N.ASTNode = DynTypedNode::create(*ASTNode);
    assert(!N.ASTNode.getNodeKind().isNone() &&
           "Expected nodes to have a valid kind.");
    if (Parent.isValid()) {
      Node &P = Tree.getMutableNode(Parent);
      P.Children.push_back(MyId);
    }
    Parent = MyId;
    ++Id;
    ++Depth;
    return std::make_tuple(MyId, Tree.getNode(MyId).Parent);
  }
  void PostTraverse(std::tuple<NodeId, NodeId> State) {
    NodeId MyId, PreviousParent;
    std::tie(MyId, PreviousParent) = State;
    assert(MyId.isValid() && "Expecting to only traverse valid nodes.");
    Parent = PreviousParent;
    --Depth;
    Node &N = Tree.getMutableNode(MyId);
    N.RightMostDescendant = Id - 1;
    assert(N.RightMostDescendant >= Tree.getRootId() &&
           N.RightMostDescendant < Tree.getSize() &&
           "Rightmost descendant must be a valid tree node.");
    if (N.isLeaf())
      Tree.Leaves.push_back(MyId);
    N.Height = 1;
    for (NodeId Child : N.Children)
      N.Height = std::max(N.Height, 1 + Tree.getNode(Child).Height);
  }
  bool TraverseDecl(Decl *D) {
    if (isNodeExcluded(Tree.AST.getSourceManager(), D))
      return true;
    auto SavedState = PreTraverse(D);
    RecursiveASTVisitor<PreorderVisitor>::TraverseDecl(D);
    PostTraverse(SavedState);
    return true;
  }
  bool TraverseStmt(Stmt *S) {
    if (S)
      S = S->IgnoreImplicit();
    if (isNodeExcluded(Tree.AST.getSourceManager(), S))
      return true;
    auto SavedState = PreTraverse(S);
    RecursiveASTVisitor<PreorderVisitor>::TraverseStmt(S);
    PostTraverse(SavedState);
    return true;
  }
  bool TraverseType(QualType T) { return true; }
  bool TraverseConstructorInitializer(CXXCtorInitializer *Init) {
    if (isNodeExcluded(Tree.AST.getSourceManager(), Init))
      return true;
    auto SavedState = PreTraverse(Init);
    RecursiveASTVisitor<PreorderVisitor>::TraverseConstructorInitializer(Init);
    PostTraverse(SavedState);
    return true;
  }
};
} // end anonymous namespace

SyntaxTree::Impl::Impl(SyntaxTree *Parent, ASTContext &AST)
    : Parent(Parent), AST(AST), TypePP(AST.getLangOpts()), Leaves(*this),
      NodesBfs(*this) {
  TypePP.AnonymousTagLocations = false;
}

SyntaxTree::Impl::Impl(SyntaxTree *Parent, Decl *N, ASTContext &AST)
    : Impl(Parent, AST) {
  PreorderVisitor PreorderWalker(*this);
  PreorderWalker.TraverseDecl(N);
  initTree();
}

SyntaxTree::Impl::Impl(SyntaxTree *Parent, Stmt *N, ASTContext &AST)
    : Impl(Parent, AST) {
  PreorderVisitor PreorderWalker(*this);
  PreorderWalker.TraverseStmt(N);
  initTree();
}

static std::vector<NodeId> getSubtreePostorder(SyntaxTree::Impl &Tree,
                                               NodeId Root) {
  std::vector<NodeId> Postorder;
  std::function<void(NodeId)> Traverse = [&](NodeId Id) {
    NodeRef N = Tree.getNode(Id);
    for (NodeId Child : N.Children)
      Traverse(Child);
    Postorder.push_back(Id);
  };
  Traverse(Root);
  return Postorder;
}

static void getSubtreeBfs(NodeList &Ids, NodeRef Root) {
  size_t Expanded = 0;
  Ids.push_back(Root.getId());
  while (Expanded < Ids.size())
    for (NodeRef Child : Ids[Expanded++])
      Ids.push_back(Child.getId());
}

void SyntaxTree::Impl::initTree() {
  setLeftMostDescendants();
  int PostorderId = 0;
  PostorderIds.resize(getSize());
  std::function<void(NodeRef)> PostorderTraverse = [&](NodeRef N) {
    for (NodeRef Child : N)
      PostorderTraverse(Child);
    PostorderIds[N.getId()] = PostorderId;
    ++PostorderId;
  };
  PostorderTraverse(getRoot());
  getSubtreeBfs(NodesBfs, getRoot());
}

void SyntaxTree::Impl::setLeftMostDescendants() {
  for (NodeRef Leaf : Leaves) {
    getMutableNode(Leaf).LeftMostDescendant = Leaf.getId();
    const Node *Parent, *Cur = &Leaf;
    while ((Parent = Cur->getParent()) && &Parent->getChild(0) == Cur) {
      Cur = Parent;
      getMutableNode(*Cur).LeftMostDescendant = Leaf.getId();
    }
  }
}

int SyntaxTree::Impl::getNumberOfDescendants(NodeRef N) const {
  return N.RightMostDescendant - N.getId() + 1;
}

bool SyntaxTree::Impl::isInSubtree(NodeRef N, NodeRef SubtreeRoot) const {
  return N.getId() >= SubtreeRoot.getId() &&
         N.getId() <= SubtreeRoot.RightMostDescendant;
}

int SyntaxTree::Impl::findPositionInParent(NodeRef N, bool Shifted) const {
  if (!N.getParent())
    return 0;
  NodeRef Parent = *N.getParent();
  const auto &Siblings = Parent.Children;
  int Position = 0;
  for (size_t I = 0, E = Siblings.size(); I < E; ++I) {
    if (Shifted)
      Position += getNode(Siblings[I]).Shift;
    if (Siblings[I] == N.getId()) {
      Position += I;
      return Position;
    }
  }
  llvm_unreachable("Node not found in parent's children.");
}

// Returns the qualified name of ND. If it is subordinate to Context,
// then the prefix of the latter is removed from the returned value.
std::string
SyntaxTree::Impl::getRelativeName(const NamedDecl *ND,
                                  const DeclContext *Context) const {
  std::string Val = ND->getQualifiedNameAsString();
  std::string ContextPrefix;
  if (!Context)
    return Val;
  if (auto *Namespace = dyn_cast<NamespaceDecl>(Context))
    ContextPrefix = Namespace->getQualifiedNameAsString();
  else if (auto *Record = dyn_cast<RecordDecl>(Context))
    ContextPrefix = Record->getQualifiedNameAsString();
  else if (AST.getLangOpts().CPlusPlus11)
    if (auto *Tag = dyn_cast<TagDecl>(Context))
      ContextPrefix = Tag->getQualifiedNameAsString();
  // Strip the qualifier, if Val refers to somthing in the current scope.
  // But leave one leading ':' in place, so that we know that this is a
  // relative path.
  if (!ContextPrefix.empty() && StringRef(Val).startswith(ContextPrefix))
    Val = Val.substr(ContextPrefix.size() + 1);
  return Val;
}

std::string SyntaxTree::Impl::getRelativeName(const NamedDecl *ND) const {
  return getRelativeName(ND, ND->getDeclContext());
}

static const DeclContext *getEnclosingDeclContext(ASTContext &AST,
                                                  const Stmt *S) {
  while (S) {
    const auto &Parents = AST.getParents(*S);
    if (Parents.empty())
      return nullptr;
    const auto &P = Parents[0];
    if (const auto *D = P.get<Decl>())
      return D->getDeclContext();
    S = P.get<Stmt>();
  }
  return nullptr;
}

static std::string getInitializerValue(const CXXCtorInitializer *Init,
                                       const PrintingPolicy &TypePP) {
  if (Init->isAnyMemberInitializer())
    return Init->getAnyMember()->getName();
  if (Init->isBaseInitializer())
    return QualType(Init->getBaseClass(), 0).getAsString(TypePP);
  if (Init->isDelegatingInitializer())
    return Init->getTypeSourceInfo()->getType().getAsString(TypePP);
  llvm_unreachable("Unknown initializer type");
}

std::string SyntaxTree::Impl::getNodeValue(NodeRef N) const {
  assert(&N.Tree == this);
  const DynTypedNode &DTN = N.ASTNode;
  if (auto *S = DTN.get<Stmt>())
    return getStmtValue(S);
  if (auto *D = DTN.get<Decl>())
    return getDeclValue(D);
  if (auto *Init = DTN.get<CXXCtorInitializer>())
    return getInitializerValue(Init, TypePP);
  llvm_unreachable("Fatal: unhandled AST node.\n");
}

std::string SyntaxTree::Impl::getDeclValue(const Decl *D) const {
  std::string Value;
  if (auto *V = dyn_cast<ValueDecl>(D))
    return getRelativeName(V) + "(" + V->getType().getAsString(TypePP) + ")";
  if (auto *N = dyn_cast<NamedDecl>(D))
    Value += getRelativeName(N) + ";";
  if (auto *T = dyn_cast<TypedefNameDecl>(D))
    return Value + T->getUnderlyingType().getAsString(TypePP) + ";";
  if (auto *T = dyn_cast<TypeDecl>(D))
    if (T->getTypeForDecl())
      Value +=
          T->getTypeForDecl()->getCanonicalTypeInternal().getAsString(TypePP) +
          ";";
  if (auto *U = dyn_cast<UsingDirectiveDecl>(D))
    return U->getNominatedNamespace()->getName();
  if (auto *A = dyn_cast<AccessSpecDecl>(D)) {
    CharSourceRange Range(A->getSourceRange(), false);
    return Lexer::getSourceText(Range, AST.getSourceManager(),
                                AST.getLangOpts());
  }
  return Value;
}

std::string SyntaxTree::Impl::getStmtValue(const Stmt *S) const {
  if (auto *U = dyn_cast<UnaryOperator>(S))
    return UnaryOperator::getOpcodeStr(U->getOpcode());
  if (auto *B = dyn_cast<BinaryOperator>(S))
    return B->getOpcodeStr();
  if (auto *M = dyn_cast<MemberExpr>(S))
    return getRelativeName(M->getMemberDecl());
  if (auto *I = dyn_cast<IntegerLiteral>(S)) {
    SmallString<256> Str;
    I->getValue().toString(Str, /*Radix=*/10, /*Signed=*/false);
    return Str.str();
  }
  if (auto *F = dyn_cast<FloatingLiteral>(S)) {
    SmallString<256> Str;
    F->getValue().toString(Str);
    return Str.str();
  }
  if (auto *D = dyn_cast<DeclRefExpr>(S))
    return getRelativeName(D->getDecl(), getEnclosingDeclContext(AST, S));
  if (auto *String = dyn_cast<StringLiteral>(S))
    return String->getString();
  if (auto *B = dyn_cast<CXXBoolLiteralExpr>(S))
    return B->getValue() ? "true" : "false";
  return "";
}

/// Identifies a node in a subtree by its postorder offset, starting at 1.
struct SNodeId {
  int Id = 0;

  explicit SNodeId(int Id) : Id(Id) {}
  explicit SNodeId() = default;

  operator int() const { return Id; }
  SNodeId &operator++() { return ++Id, *this; }
  SNodeId &operator--() { return --Id, *this; }
  SNodeId operator+(int Other) const { return SNodeId(Id + Other); }
};

class Subtree {
private:
  /// The parent tree.
  SyntaxTree::Impl &Tree;
  /// Maps SNodeIds to original ids.
  std::vector<NodeId> RootIds;
  /// Maps subtree nodes to their leftmost descendants wtihin the subtree.
  std::vector<SNodeId> LeftMostDescendants;

public:
  std::vector<SNodeId> KeyRoots;

  Subtree(SyntaxTree::Impl &Tree, NodeId SubtreeRoot) : Tree(Tree) {
    RootIds = getSubtreePostorder(Tree, SubtreeRoot);
    int NumLeaves = setLeftMostDescendants();
    computeKeyRoots(NumLeaves);
  }
  int getSize() const { return RootIds.size(); }
  NodeId getIdInRoot(SNodeId Id) const {
    assert(Id > 0 && Id <= getSize() && "Invalid subtree node index.");
    return RootIds[Id - 1];
  }
  NodeRef getNode(SNodeId Id) const { return Tree.getNode(getIdInRoot(Id)); }
  SNodeId getLeftMostDescendant(SNodeId Id) const {
    assert(Id > 0 && Id <= getSize() && "Invalid subtree node index.");
    return LeftMostDescendants[Id - 1];
  }
  /// Returns the postorder index of the leftmost descendant in the subtree.
  NodeId getPostorderOffset() const {
    return Tree.PostorderIds[getIdInRoot(SNodeId(1))];
  }
  std::string getNodeValue(SNodeId Id) const {
    return Tree.getNodeValue(getNode(Id));
  }

private:
  /// Returns the number of leafs in the subtree.
  int setLeftMostDescendants() {
    int NumLeaves = 0;
    LeftMostDescendants.resize(getSize());
    for (int I = 0; I < getSize(); ++I) {
      SNodeId SI(I + 1);
      NodeRef N = getNode(SI);
      NumLeaves += N.isLeaf();
      assert(I == Tree.PostorderIds[getIdInRoot(SI)] - getPostorderOffset() &&
             "Postorder traversal in subtree should correspond to traversal in "
             "the root tree by a constant offset.");
      LeftMostDescendants[I] = SNodeId(Tree.PostorderIds[N.LeftMostDescendant] -
                                       getPostorderOffset());
    }
    return NumLeaves;
  }
  void computeKeyRoots(int Leaves) {
    KeyRoots.resize(Leaves);
    std::unordered_set<int> Visited;
    int K = Leaves - 1;
    for (SNodeId I(getSize()); I > 0; --I) {
      SNodeId LeftDesc = getLeftMostDescendant(I);
      if (Visited.count(LeftDesc))
        continue;
      assert(K >= 0 && "K should be non-negative");
      KeyRoots[K] = I;
      Visited.insert(LeftDesc);
      --K;
    }
  }
};

/// Implementation of Zhang and Shasha's Algorithm for tree edit distance.
/// Computes an optimal mapping between two trees using only insertion,
/// deletion and update as edit actions (similar to the Levenshtein distance).
class ZhangShashaMatcher {
  const ASTDiff::Impl &DiffImpl;
  Subtree S1;
  Subtree S2;
  std::unique_ptr<std::unique_ptr<double[]>[]> TreeDist, ForestDist;

public:
  ZhangShashaMatcher(const ASTDiff::Impl &DiffImpl, SyntaxTree::Impl &T1,
                     SyntaxTree::Impl &T2, NodeId Id1, NodeId Id2)
      : DiffImpl(DiffImpl), S1(T1, Id1), S2(T2, Id2) {
    TreeDist = llvm::make_unique<std::unique_ptr<double[]>[]>(
        size_t(S1.getSize()) + 1);
    ForestDist = llvm::make_unique<std::unique_ptr<double[]>[]>(
        size_t(S1.getSize()) + 1);
    for (int I = 0, E = S1.getSize() + 1; I < E; ++I) {
      TreeDist[I] = llvm::make_unique<double[]>(size_t(S2.getSize()) + 1);
      ForestDist[I] = llvm::make_unique<double[]>(size_t(S2.getSize()) + 1);
    }
  }

  std::vector<std::pair<NodeId, NodeId>> getMatchingNodes() {
    std::vector<std::pair<NodeId, NodeId>> Matches;
    std::vector<std::pair<SNodeId, SNodeId>> TreePairs;

    computeTreeDist();

    bool RootNodePair = true;

    TreePairs.emplace_back(SNodeId(S1.getSize()), SNodeId(S2.getSize()));

    while (!TreePairs.empty()) {
      SNodeId LastRow, LastCol, FirstRow, FirstCol, Row, Col;
      std::tie(LastRow, LastCol) = TreePairs.back();
      TreePairs.pop_back();

      if (!RootNodePair) {
        computeForestDist(LastRow, LastCol);
      }

      RootNodePair = false;

      FirstRow = S1.getLeftMostDescendant(LastRow);
      FirstCol = S2.getLeftMostDescendant(LastCol);

      Row = LastRow;
      Col = LastCol;

      while (Row > FirstRow || Col > FirstCol) {
        if (Row > FirstRow &&
            ForestDist[Row - 1][Col] + 1 == ForestDist[Row][Col]) {
          --Row;
        } else if (Col > FirstCol &&
                   ForestDist[Row][Col - 1] + 1 == ForestDist[Row][Col]) {
          --Col;
        } else {
          SNodeId LMD1 = S1.getLeftMostDescendant(Row);
          SNodeId LMD2 = S2.getLeftMostDescendant(Col);
          if (LMD1 == S1.getLeftMostDescendant(LastRow) &&
              LMD2 == S2.getLeftMostDescendant(LastCol)) {
            NodeRef N1 = S1.getNode(Row);
            NodeRef N2 = S2.getNode(Col);
            assert(DiffImpl.isMatchingPossible(N1, N2) &&
                   "These nodes must not be matched.");
            Matches.emplace_back(N1.getId(), N2.getId());
            --Row;
            --Col;
          } else {
            TreePairs.emplace_back(Row, Col);
            Row = LMD1;
            Col = LMD2;
          }
        }
      }
    }
    return Matches;
  }

private:
  /// We use a simple cost model for edit actions, which seems good enough.
  /// Simple cost model for edit actions. This seems to make the matching
  /// algorithm perform reasonably well.
  /// The values range between 0 and 1, or infinity if this edit action should
  /// always be avoided.
  static constexpr double DeletionCost = 1;
  static constexpr double InsertionCost = 1;

  double getUpdateCost(SNodeId Id1, SNodeId Id2) {
    NodeRef N1 = S1.getNode(Id1), N2 = S2.getNode(Id2);
    if (!DiffImpl.isMatchingPossible(N1, N2))
      return std::numeric_limits<double>::max();
    return S1.getNodeValue(Id1) != S2.getNodeValue(Id2);
  }

  void computeTreeDist() {
    for (SNodeId Id1 : S1.KeyRoots)
      for (SNodeId Id2 : S2.KeyRoots)
        computeForestDist(Id1, Id2);
  }

  void computeForestDist(SNodeId Id1, SNodeId Id2) {
    assert(Id1 > 0 && Id2 > 0 && "Expecting offsets greater than 0.");
    SNodeId LMD1 = S1.getLeftMostDescendant(Id1);
    SNodeId LMD2 = S2.getLeftMostDescendant(Id2);

    ForestDist[LMD1][LMD2] = 0;
    for (SNodeId D1 = LMD1 + 1; D1 <= Id1; ++D1) {
      ForestDist[D1][LMD2] = ForestDist[D1 - 1][LMD2] + DeletionCost;
      for (SNodeId D2 = LMD2 + 1; D2 <= Id2; ++D2) {
        ForestDist[LMD1][D2] = ForestDist[LMD1][D2 - 1] + InsertionCost;
        SNodeId DLMD1 = S1.getLeftMostDescendant(D1);
        SNodeId DLMD2 = S2.getLeftMostDescendant(D2);
        if (DLMD1 == LMD1 && DLMD2 == LMD2) {
          double UpdateCost = getUpdateCost(D1, D2);
          ForestDist[D1][D2] =
              std::min({ForestDist[D1 - 1][D2] + DeletionCost,
                        ForestDist[D1][D2 - 1] + InsertionCost,
                        ForestDist[D1 - 1][D2 - 1] + UpdateCost});
          TreeDist[D1][D2] = ForestDist[D1][D2];
        } else {
          ForestDist[D1][D2] =
              std::min({ForestDist[D1 - 1][D2] + DeletionCost,
                        ForestDist[D1][D2 - 1] + InsertionCost,
                        ForestDist[DLMD1][DLMD2] + TreeDist[D1][D2]});
        }
      }
    }
  }
};

NodeId Node::getId() const { return this - &Tree.getRoot(); }
SyntaxTree &Node::getTree() const { return *Tree.Parent; }
const Node *Node::getParent() const {
  if (Parent.isInvalid())
    return nullptr;
  return &Tree.getNode(Parent);
}

NodeRef Node::getChild(size_t Index) const {
  return Tree.getNode(Children[Index]);
}

ast_type_traits::ASTNodeKind Node::getType() const {
  return ASTNode.getNodeKind();
}

StringRef Node::getTypeLabel() const { return getType().asStringRef(); }

llvm::Optional<std::string> Node::getQualifiedIdentifier() const {
  if (auto *ND = ASTNode.get<NamedDecl>()) {
    if (ND->getDeclName().isIdentifier())
      return ND->getQualifiedNameAsString();
  }
  return llvm::None;
}

llvm::Optional<StringRef> Node::getIdentifier() const {
  if (auto *ND = ASTNode.get<NamedDecl>()) {
    if (ND->getDeclName().isIdentifier())
      return ND->getName();
  }
  return llvm::None;
}

NodeRefIterator Node::begin() const {
  return {&Tree, isLeaf() ? nullptr : &Children[0]};
}
NodeRefIterator Node::end() const {
  return {&Tree, isLeaf() ? nullptr : &Children[0] + Children.size()};
}

namespace {
// Compares nodes by their depth.
struct HeightLess {
  SyntaxTree::Impl &Tree;
  HeightLess(SyntaxTree::Impl &Tree) : Tree(Tree) {}
  bool operator()(NodeId Id1, NodeId Id2) const {
    return Tree.getNode(Id1).Height < Tree.getNode(Id2).Height;
  }
};
} // end anonymous namespace

namespace {
// Priority queue for nodes, sorted descendingly by their height.
class PriorityList {
  SyntaxTree::Impl &Tree;
  HeightLess Cmp;
  std::vector<NodeId> Container;
  PriorityQueue<NodeId, std::vector<NodeId>, HeightLess> List;

public:
  PriorityList(SyntaxTree::Impl &Tree)
      : Tree(Tree), Cmp(Tree), List(Cmp, Container) {}

  void push(NodeId Id) { List.push(Id); }

  NodeList pop() {
    int Max = peekMax();
    NodeList Result(Tree);
    if (Max == 0)
      return Result;
    while (peekMax() == Max) {
      Result.push_back(List.top());
      List.pop();
    }
    // TODO this is here to get a stable output, not a good heuristic
    Result.sort();
    return Result;
  }
  int peekMax() const {
    if (List.empty())
      return 0;
    return Tree.getNode(List.top()).Height;
  }
  void open(NodeRef N) {
    for (NodeRef Child : N)
      push(Child.getId());
  }
};
} // end anonymous namespace

bool ASTDiff::Impl::identical(NodeRef N1, NodeRef N2) const {
  if (N1.getNumChildren() != N2.getNumChildren() ||
      !isMatchingPossible(N1, N2) || T1.getNodeValue(N1) != T2.getNodeValue(N2))
    return false;
  for (size_t Id = 0, E = N1.getNumChildren(); Id < E; ++Id)
    if (!identical(N1.getChild(Id), N2.getChild(Id)))
      return false;
  return true;
}

bool ASTDiff::Impl::isMatchingPossible(NodeRef N1, NodeRef N2) const {
  return Options.isMatchingAllowed(N1, N2);
}

bool ASTDiff::Impl::haveSameParents(const Mapping &M, NodeRef N1,
                                    NodeRef N2) const {
  const Node *P1 = N1.getParent();
  const Node *P2 = N2.getParent();
  return (!P1 && !P2) || (P1 && P2 && M.getDst(P1->getId()) == P2->getId());
}

void ASTDiff::Impl::addOptimalMapping(Mapping &M, NodeRef N1,
                                      NodeRef N2) const {
  if (std::max(T1.getNumberOfDescendants(N1), T2.getNumberOfDescendants(N2)) >
      Options.MaxSize)
    return;
  ZhangShashaMatcher Matcher(*this, T1, T2, N1.getId(), N2.getId());
  std::vector<std::pair<NodeId, NodeId>> R = Matcher.getMatchingNodes();
  for (const auto Tuple : R) {
    NodeRef N1 = T1.getNode(Tuple.first);
    NodeRef N2 = T2.getNode(Tuple.second);
    if (!M.hasSrc(N1.getId()) && !M.hasDst(N2.getId()))
      M.link(N1.getId(), N2.getId());
  }
}

double ASTDiff::Impl::getJaccardSimilarity(const Mapping &M, NodeRef N1,
                                           NodeRef N2) const {
  int CommonDescendants = 0;
  // Count the common descendants, excluding the subtree root.
  for (NodeId Src = N1.getId() + 1; Src <= N1.RightMostDescendant; ++Src) {
    const Node *Dst = getMapped(T1.Parent->TreeImpl, T1.getNode(Src));
    if (Dst)
      CommonDescendants += T2.isInSubtree(*Dst, N2);
  }
  // We need to subtract 1 to get the number of descendants excluding the
  // root.
  double Denominator = T1.getNumberOfDescendants(N1) - 1 +
                       T2.getNumberOfDescendants(N2) - 1 - CommonDescendants;
  // CommonDescendants is less than the size of one subtree.
  assert(Denominator >= 0 && "Expected non-negative denominator.");
  if (Denominator == 0)
    return 0;
  return CommonDescendants / Denominator;
}

const Node *ASTDiff::Impl::findCandidate(const Mapping &M, NodeRef N1) const {
  const Node *Candidate = nullptr;
  double HighestSimilarity = 0.0;
  for (NodeRef N2 : T2) {
    if (!isMatchingPossible(N1, N2))
      continue;
    if (M.hasDst(N2.getId()))
      continue;
    double Similarity = getJaccardSimilarity(M, N1, N2);
    if (Similarity >= Options.MinSimilarity && Similarity > HighestSimilarity) {
      HighestSimilarity = Similarity;
      Candidate = &N2;
    }
  }
  return Candidate;
}

void ASTDiff::Impl::matchBottomUp(Mapping &M) const {
  std::vector<NodeId> Postorder = getSubtreePostorder(T1, T1.getRootId());
  for (NodeId Id1 : Postorder) {
    if (Id1 == T1.getRootId() && !M.hasSrc(T1.getRootId()) &&
        !M.hasDst(T2.getRootId())) {
      if (isMatchingPossible(T1.getRoot(), T2.getRoot())) {
        M.link(T1.getRootId(), T2.getRootId());
        addOptimalMapping(M, T1.getRoot(), T2.getRoot());
      }
      break;
    }
    bool Matched = M.hasSrc(Id1);
    NodeRef N1 = T1.getNode(Id1);
    bool MatchedChildren =
        std::any_of(N1.Children.begin(), N1.Children.end(),
                    [&](NodeId Child) { return M.hasSrc(Child); });
    if (Matched || !MatchedChildren)
      continue;
    const Node *N2 = findCandidate(M, N1);
    if (N2) {
      M.link(N1.getId(), N2->getId());
      addOptimalMapping(M, N1, *N2);
    }
  }
}

Mapping ASTDiff::Impl::matchTopDown() {
  PriorityList L1(T1);
  PriorityList L2(T2);

  Mapping M(T1.getSize() + T2.getSize());

  L1.push(T1.getRootId());
  L2.push(T2.getRootId());

  int Max1, Max2;
  while (std::min(Max1 = L1.peekMax(), Max2 = L2.peekMax()) >
         Options.MinHeight) {
    if (Max1 > Max2) {
      for (NodeRef N1 : L1.pop())
        L1.open(N1);
      continue;
    }
    if (Max2 > Max1) {
      for (NodeRef N2 : L2.pop())
        L2.open(N2);
      continue;
    }
    NodeList H1 = L1.pop(), H2 = L2.pop();
    for (NodeRef N1 : H1) {
      for (NodeRef N2 : H2) {
        if (identical(N1, N2) && !M.hasSrc(N1.getId()) &&
            !M.hasDst(N2.getId())) {
          for (int I = 0, E = T1.getNumberOfDescendants(N1); I < E; ++I)
            M.link(N1.getId() + I, N2.getId() + I);
        }
      }
    }
    for (NodeRef N1 : H1) {
      if (!M.hasSrc(N1.getId()))
        L1.open(N1);
    }
    for (NodeRef N2 : H2) {
      if (!M.hasDst(N2.getId()))
        L2.open(N2);
    }
  }
  return M;
}

ASTDiff::Impl::Impl(SyntaxTree::Impl &T1, SyntaxTree::Impl &T2,
                    const ComparisonOptions &Options)
    : T1(T1), T2(T2), Options(Options) {
  computeMapping();
  computeChangeKinds(TheMapping);
}

void ASTDiff::Impl::computeMapping() {
  TheMapping = matchTopDown();
  if (Options.StopAfterTopDown)
    return;
  matchBottomUp(TheMapping);
}

void ASTDiff::Impl::computeChangeKinds(Mapping &M) {
  for (NodeRef N1 : T1) {
    if (!M.hasSrc(N1.getId())) {
      T1.getMutableNode(N1.getId()).Change = Delete;
      T1.getMutableNode(N1.getId()).Shift -= 1;
    }
  }
  for (NodeRef N2 : T2) {
    if (!M.hasDst(N2.getId())) {
      T2.getMutableNode(N2.getId()).Change = Insert;
      T2.getMutableNode(N2.getId()).Shift -= 1;
    }
  }
  for (NodeRef N1 : T1.NodesBfs) {
    NodeId Id2 = M.getDst(N1.getId());
    if (Id2.isInvalid())
      continue;
    NodeRef N2 = T2.getNode(Id2);
    if (!haveSameParents(M, N1, N2) || T1.findPositionInParent(N1, true) !=
                                           T2.findPositionInParent(N2, true)) {
      T1.getMutableNode(N1).Shift -= 1;
      T2.getMutableNode(N2).Shift -= 1;
    }
  }
  for (NodeRef N2TODO : T2.NodesBfs) {
    NodeId Id1 = M.getSrc(N2TODO.getId());
    if (Id1.isInvalid())
      continue;
    Node &N1 = T1.getMutableNode(Id1);
    Node &N2 = T2.getMutableNode(N2TODO.getId());
    if (Id1.isInvalid())
      continue;
    if (!haveSameParents(M, N1, N2) || T1.findPositionInParent(N1, true) !=
                                           T2.findPositionInParent(N2, true)) {
      N1.Change = N2.Change = Move;
    }
    if (T1.getNodeValue(N1) != T2.getNodeValue(N2)) {
      N1.Change = N2.Change = (N1.Change == Move ? UpdateMove : Update);
    }
  }
}

const Node *
ASTDiff::Impl::getMapped(const std::unique_ptr<SyntaxTree::Impl> &Tree,
                         NodeRef N) const {
  if (&*Tree == &T1) {
    NodeId Id = TheMapping.getDst(N.getId());
    return Id.isValid() ? &T2.getNode(Id) : nullptr;
  }
  assert(&*Tree == &T2 && "Invalid tree.");
  NodeId Id = TheMapping.getSrc(N.getId());
  return Id.isValid() ? &T1.getNode(Id) : nullptr;
}

ASTDiff::ASTDiff(SyntaxTree &T1, SyntaxTree &T2,
                 const ComparisonOptions &Options)
    : DiffImpl(llvm::make_unique<Impl>(*T1.TreeImpl, *T2.TreeImpl, Options)) {}

ASTDiff::~ASTDiff() = default;

const Node *ASTDiff::getMapped(const SyntaxTree &SourceTree, NodeRef N) const {
  return DiffImpl->getMapped(SourceTree.TreeImpl, N);
}

SyntaxTree::SyntaxTree(ASTContext &AST)
    : TreeImpl(llvm::make_unique<SyntaxTree::Impl>(
          this, AST.getTranslationUnitDecl(), AST)) {}

SyntaxTree::~SyntaxTree() = default;

const ASTContext &SyntaxTree::getASTContext() const { return TreeImpl->AST; }

NodeRef SyntaxTree::getNode(NodeId Id) const { return TreeImpl->getNode(Id); }

int SyntaxTree::getSize() const { return TreeImpl->getSize(); }
NodeRef SyntaxTree::getRoot() const { return TreeImpl->getRoot(); }
SyntaxTree::PreorderIterator SyntaxTree::begin() const {
  return TreeImpl->begin();
}
SyntaxTree::PreorderIterator SyntaxTree::end() const { return TreeImpl->end(); }

int SyntaxTree::findPositionInParent(NodeRef N) const {
  return TreeImpl->findPositionInParent(N);
}

std::pair<unsigned, unsigned>
SyntaxTree::getSourceRangeOffsets(NodeRef N) const {
  const SourceManager &SM = TreeImpl->AST.getSourceManager();
  SourceRange Range = N.ASTNode.getSourceRange();
  SourceLocation BeginLoc = Range.getBegin();
  SourceLocation EndLoc = Lexer::getLocForEndOfToken(
      Range.getEnd(), /*Offset=*/0, SM, TreeImpl->AST.getLangOpts());
  if (auto *ThisExpr = N.ASTNode.get<CXXThisExpr>()) {
    if (ThisExpr->isImplicit())
      EndLoc = BeginLoc;
  }
  unsigned Begin = SM.getFileOffset(SM.getExpansionLoc(BeginLoc));
  unsigned End = SM.getFileOffset(SM.getExpansionLoc(EndLoc));
  return {Begin, End};
}

std::string SyntaxTree::getNodeValue(NodeRef N) const {
  return TreeImpl->getNodeValue(N);
}

} // end namespace diff
} // end namespace clang
