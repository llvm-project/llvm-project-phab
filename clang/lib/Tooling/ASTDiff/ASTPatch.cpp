//===- ASTPatch.cpp - Structural patching based on ASTDiff ----*- C++ -*- -===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "clang/Tooling/ASTDiff/ASTPatch.h"

#include "clang/AST/DeclTemplate.h"
#include "clang/AST/ExprCXX.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/Core/Replacement.h"

using namespace llvm;
using namespace clang;
using namespace tooling;

namespace clang {
namespace diff {

static Error error(patching_error code) {
  return llvm::make_error<PatchingError>(code);
};

static CharSourceRange makeEmptyCharRange(SourceLocation Point) {
  return CharSourceRange::getCharRange(Point, Point);
}

// Returns a comparison function that considers invalid source locations
// to be less than anything.
static std::function<bool(SourceLocation, SourceLocation)>
makeTolerantLess(SourceManager &SM) {
  return [&SM](SourceLocation A, SourceLocation B) {
    if (A.isInvalid())
      return true;
    if (B.isInvalid())
      return false;
    BeforeThanCompare<SourceLocation> Less(SM);
    return Less(A, B);
  };
}

namespace {
// This wraps a node from Patcher::Target or Patcher::Dst.
class PatchedTreeNode {
  NodeRef BaseNode;

public:
  operator NodeRef() const { return BaseNode; }
  NodeRef originalNode() const { return *this; }
  CharSourceRange getSourceRange() const { return BaseNode.getSourceRange(); }
  NodeId getId() const { return BaseNode.getId(); }
  SyntaxTree &getTree() const { return BaseNode.getTree(); }
  StringRef getTypeLabel() const { return BaseNode.getTypeLabel(); }
  decltype(BaseNode.getOwnedSourceRanges()) getOwnedSourceRanges() {
    return BaseNode.getOwnedSourceRanges();
  }
  decltype(BaseNode.getOwnedTokens()) getOwnedTokens() {
    return BaseNode.getOwnedTokens();
  }

  // This flag indicates whether this node, or any of its descendants was
  // changed with regards to the original tree.
  bool Changed = false;
  // The pointers to the children, including nodes that have been inserted or
  // moved here.
  SmallVector<PatchedTreeNode *, 4> Children;
  // First location for each child.
  SmallVector<SourceLocation, 4> ChildrenLocations;
  // The offsets at which the children should be inserted into OwnText.
  SmallVector<unsigned, 4> ChildrenOffsets;

  // This contains the text of this node, but not the text of it's children.
  Optional<std::string> OwnText;

  PatchedTreeNode(NodeRef BaseNode) : BaseNode(BaseNode) {}
  PatchedTreeNode(const PatchedTreeNode &Other) = delete;
  PatchedTreeNode(PatchedTreeNode &&Other) = default;

  void addInsertion(PatchedTreeNode &PatchedNode, SourceLocation InsertionLoc) {
    addChildAt(PatchedNode, InsertionLoc);
    SplitAt.emplace_back(InsertionLoc);
  }
  void addChild(PatchedTreeNode &PatchedNode) {
    SourceLocation InsertionLoc = PatchedNode.getSourceRange().getBegin();
    addChildAt(PatchedNode, InsertionLoc);
  }

  // This returns an array of source ranges identical to the input,
  // except whenever a SourceRange contains any of the locations in
  // SplitAt, it is split up in two ranges at that location.
  SmallVector<CharSourceRange, 4>
  splitSourceRanges(ArrayRef<CharSourceRange> SourceRanges);

private:
  void addChildAt(PatchedTreeNode &PatchedNode, SourceLocation InsertionLoc) {
    auto Less = makeTolerantLess(getTree().getSourceManager());
    auto It = std::lower_bound(ChildrenLocations.begin(),
                               ChildrenLocations.end(), InsertionLoc, Less);
    auto Offset = It - ChildrenLocations.begin();
    Children.insert(Children.begin() + Offset, &PatchedNode);
    ChildrenLocations.insert(It, InsertionLoc);
  }

  SmallVector<SourceLocation, 2> SplitAt;
};
} // end anonymous namespace

namespace {
struct TokenInsertion {
  enum { BeforeFirst = -1 };
  int AfterToken = BeforeFirst;
  int AfterChild = BeforeFirst;
  unsigned Token;
};
} // end anonymous namespace

namespace {
struct TokenMapping {
  SmallVector<unsigned, 4> DeletedTokens;
  SmallVector<TokenInsertion, 4> Insertions;
  SmallVector<int, 4> Mapping;
};
} // end anonymous namespace

namespace {
class Patcher {
  SyntaxTree &Dst, &Target;
  SourceManager &SM;
  const LangOptions &LangOpts;
  BeforeThanCompare<SourceLocation> Less;
  ASTDiff Diff, TargetDiff;
  RefactoringTool &TargetTool;
  bool Debug;
  std::vector<PatchedTreeNode> PatchedTreeNodes;
  std::map<NodeId, PatchedTreeNode *> InsertedNodes;
  // Maps NodeId in Dst to a flag that is true if this node is
  // part of an inserted subtree.
  std::vector<bool> AtomicInsertions;

public:
  Patcher(SyntaxTree &Src, SyntaxTree &Dst, SyntaxTree &Target,
          const ComparisonOptions &Options, RefactoringTool &TargetTool,
          bool Debug)
      : Dst(Dst), Target(Target), SM(Target.getSourceManager()),
        LangOpts(Target.getLangOpts()), Less(SM), Diff(Src, Dst, Options),
        TargetDiff(Src, Target, Options), TargetTool(TargetTool), Debug(Debug) {
  }

  Error apply();

private:
  void buildPatchedTree();
  void addInsertedAndMovedNodes();
  SourceLocation findLocationForInsertion(NodeRef &InsertedNode,
                                          PatchedTreeNode &InsertionTarget);
  SourceLocation findLocationForMove(NodeRef DstNode, NodeRef TargetNode,
                                     PatchedTreeNode &NewParent);
  void markChangedNodes();
  Error addReplacementsForChangedNodes();
  Error addReplacementsForTopLevelChanges();

  // Recursively builds the text that is represented by this subtree.
  std::string buildSourceText(PatchedTreeNode &PatchedNode);
  void setOwnedSourceText(PatchedTreeNode &PatchedNode);
  // Uses a LCS algorithm to determine insertions and deletions of tokens.
  TokenMapping matchTokens(NodeRef InsertionDestination,
                           NodeRef InsertionSource);
  std::pair<int, bool>
  findPointOfInsertion(NodeRef N, PatchedTreeNode &TargetParent) const;
  bool isInserted(const PatchedTreeNode &PatchedNode) const {
    return isFromDst(PatchedNode);
  }
  ChangeKind getChange(NodeRef TargetNode) const {
    if (!isFromTarget(TargetNode))
      return NoChange;
    const Node *SrcNode = TargetDiff.getMapped(TargetNode);
    if (!SrcNode)
      return NoChange;
    return Diff.getNodeChange(*SrcNode);
  }
  bool isRemoved(NodeRef TargetNode) const {
    return getChange(TargetNode) == Delete;
  }
  bool isMoved(NodeRef TargetNode) const {
    return getChange(TargetNode) == Move || getChange(TargetNode) == UpdateMove;
  }
  bool isRemovedOrMoved(NodeRef TargetNode) const {
    return isRemoved(TargetNode) || isMoved(TargetNode);
  }
  PatchedTreeNode &findParent(NodeRef N) {
    if (isFromDst(N))
      return findDstParent(N);
    return findTargetParent(N);
  }
  PatchedTreeNode &findDstParent(NodeRef DstNode) {
    const Node *SrcNode = Diff.getMapped(DstNode);
    NodeRef DstParent = *DstNode.getParent();
    if (SrcNode) {
      assert(Diff.getNodeChange(*SrcNode) == Insert);
      const Node *TargetParent = mapDstToTarget(DstParent);
      assert(TargetParent);
      return getTargetPatchedNode(*TargetParent);
    }
    return getPatchedNode(DstParent);
  }
  PatchedTreeNode &findTargetParent(NodeRef TargetNode) {
    assert(isFromTarget(TargetNode));
    const Node *SrcNode = TargetDiff.getMapped(TargetNode);
    if (SrcNode) {
      ChangeKind Change = Diff.getNodeChange(*SrcNode);
      if (Change == Move || Change == UpdateMove) {
        NodeRef DstNode = *Diff.getMapped(*SrcNode);
        return getPatchedNode(*DstNode.getParent());
      }
    }
    return getTargetPatchedNode(*TargetNode.getParent());
  }
  CharSourceRange getRangeForReplacing(NodeRef TargetNode) const {
    if (isRemovedOrMoved(TargetNode))
      return TargetNode.findRangeForDeletion();
    return TargetNode.getSourceRange();
  }
  Error addReplacement(Replacement &&R) {
    return TargetTool.getReplacements()[R.getFilePath()].add(R);
  }
  bool isFromTarget(NodeRef N) const { return &N.getTree() == &Target; }
  bool isFromDst(NodeRef N) const { return &N.getTree() == &Dst; }
  PatchedTreeNode &getTargetPatchedNode(NodeRef N) {
    assert(isFromTarget(N));
    return PatchedTreeNodes[N.getId()];
  }
  PatchedTreeNode &getPatchedNode(NodeRef N) {
    if (isFromDst(N))
      return *InsertedNodes.at(N.getId());
    return PatchedTreeNodes[N.getId()];
  }
  const Node *mapDstToTarget(NodeRef DstNode) const {
    const Node *SrcNode = Diff.getMapped(DstNode);
    if (!SrcNode)
      return nullptr;
    return TargetDiff.getMapped(*SrcNode);
  }
  const Node *mapTargetToDst(NodeRef TargetNode) const {
    const Node *SrcNode = TargetDiff.getMapped(TargetNode);
    if (!SrcNode)
      return nullptr;
    return Diff.getMapped(*SrcNode);
  }
};
} // end anonymous namespace

static void markBiggestSubtrees(std::vector<bool> &Marked, SyntaxTree &Tree,
                                llvm::function_ref<bool(NodeRef)> Predicate) {
  Marked.resize(Tree.getSize());
  for (NodeRef N : Tree.postorder()) {
    bool AllChildrenMarked =
        std::all_of(N.begin(), N.end(),
                    [&Marked](NodeRef Child) { return Marked[Child.getId()]; });
    Marked[N.getId()] = Predicate(N) && AllChildrenMarked;
  }
}

Error Patcher::apply() {
  if (Debug)
    Diff.dumpChanges(llvm::errs(), /*DumpMatches=*/true);
  markBiggestSubtrees(AtomicInsertions, Dst, [this](NodeRef DstNode) {
    return Diff.getNodeChange(DstNode) == Insert;
  });
  buildPatchedTree();
  addInsertedAndMovedNodes();
  markChangedNodes();
  if (auto Err = addReplacementsForChangedNodes())
    return Err;
  Rewriter Rewrite(SM, LangOpts);
  if (!TargetTool.applyAllReplacements(Rewrite))
    return error(patching_error::failed_to_apply_replacements);
  if (Rewrite.overwriteChangedFiles())
    // Some file has not been saved successfully.
    return error(patching_error::failed_to_overwrite_files);
  return Error::success();
}

static bool wantToInsertBefore(SourceLocation Insertion, SourceLocation Point,
                               BeforeThanCompare<SourceLocation> &Less) {
  assert(Insertion.isValid());
  assert(Point.isValid());
  return Less(Insertion, Point);
}

void Patcher::buildPatchedTree() {
  // Firstly, add all nodes of the tree that will be patched to
  // PatchedTreeNodes. This way, their offset (getId()) is the same as in the
  // original tree.
  PatchedTreeNodes.reserve(Target.getSize());
  for (NodeRef TargetNode : Target)
    PatchedTreeNodes.emplace_back(TargetNode);
  // Then add all inserted nodes, from Dst.
  for (NodeId DstId = Dst.getRootId(), E = Dst.getSize(); DstId < E; ++DstId) {
    NodeRef DstNode = Dst.getNode(DstId);
    ChangeKind Change = Diff.getNodeChange(DstNode);
    if (Change == Insert) {
      PatchedTreeNodes.emplace_back(DstNode);
      InsertedNodes.emplace(DstNode.getId(), &PatchedTreeNodes.back());
      // If the whole subtree is inserted, we can skip the children, as we
      // will just copy the text of the entire subtree.
      if (AtomicInsertions[DstId])
        DstId = DstNode.RightMostDescendant;
    }
  }
  // Add existing children.
  for (auto &PatchedNode : PatchedTreeNodes) {
    if (isFromTarget(PatchedNode))
      for (auto &Child : PatchedNode.originalNode())
        if (!isRemovedOrMoved(Child))
          PatchedNode.addChild(getPatchedNode(Child));
  }
}

void Patcher::addInsertedAndMovedNodes() {
  ChangeKind Change = NoChange;
  for (NodeId DstId = Dst.getRootId(), E = Dst.getSize(); DstId < E;
       DstId = Change == Insert && AtomicInsertions[DstId]
                   ? Dst.getNode(DstId).RightMostDescendant + 1
                   : DstId + 1) {
    NodeRef DstNode = Dst.getNode(DstId);
    Change = Diff.getNodeChange(DstNode);
    if (!(Change == Move || Change == UpdateMove || Change == Insert))
      continue;
    NodeRef DstParent = *DstNode.getParent();
    PatchedTreeNode *InsertionTarget, *NodeToInsert;
    SourceLocation InsertionLoc;
    if (Diff.getNodeChange(DstParent) == Insert) {
      InsertionTarget = &getPatchedNode(DstParent);
    } else {
      const Node *TargetParent = mapDstToTarget(DstParent);
      if (!TargetParent)
        continue;
      InsertionTarget = &getTargetPatchedNode(*TargetParent);
    }
    if (Change == Insert) {
      NodeToInsert = &getPatchedNode(DstNode);
      InsertionLoc = findLocationForInsertion(DstNode, *InsertionTarget);
    } else {
      assert(Change == Move || Change == UpdateMove);
      const Node *TargetNode = mapDstToTarget(DstNode);
      assert(TargetNode && "Node to update not found.");
      NodeToInsert = &getTargetPatchedNode(*TargetNode);
      InsertionLoc =
          findLocationForMove(DstNode, *TargetNode, *InsertionTarget);
    }
    assert(InsertionLoc.isValid());
    InsertionTarget->addInsertion(*NodeToInsert, InsertionLoc);
  }
}

SourceLocation
Patcher::findLocationForInsertion(NodeRef DstNode,
                                  PatchedTreeNode &InsertionTarget) {
  assert(isFromDst(DstNode));
  assert(isFromDst(InsertionTarget) || isFromTarget(InsertionTarget));
  int ChildIndex;
  bool RightOfChild;
  unsigned NumChildren = InsertionTarget.Children.size();
  std::tie(ChildIndex, RightOfChild) =
      findPointOfInsertion(DstNode, InsertionTarget);
  if (NumChildren && ChildIndex != -1) {
    auto NeighborRange = InsertionTarget.Children[ChildIndex]->getSourceRange();
    SourceLocation InsertionLocation =
        RightOfChild ? NeighborRange.getEnd() : NeighborRange.getBegin();
    if (InsertionLocation.isValid())
      return InsertionLocation;
  }
  NodeRef DstParent = *DstNode.getParent();
  TokenMapping MapDstNew = matchTokens(DstParent, InsertionTarget);
  BeforeThanCompare<SourceLocation> DstLess(Dst.getSourceManager());
  auto InsertedLoc = DstNode.getSourceRange().getBegin();
  assert(InsertedLoc.isValid());
  auto DstTokens = DstParent.getOwnedTokens();
  auto NewTokens = InsertionTarget.getOwnedTokens();
  for (unsigned I = 0; I < NewTokens.size(); ++I) {
    int DstIndex = MapDstNew.Mapping[I];
    if (DstIndex == -1)
      continue;
    SourceLocation NewLoc = NewTokens[I];
    SourceLocation DstLoc = DstTokens[DstIndex];
    if (DstLoc.isValid() && wantToInsertBefore(InsertedLoc, DstLoc, DstLess))
      return NewLoc;
  }
  if (isFromDst(InsertionTarget))
    return InsertionTarget.getSourceRange().getEnd();
  assert(NodeRef(InsertionTarget).ASTNode.get<TranslationUnitDecl>());
  return SM.getLocForEndOfFile(SM.getMainFileID());
}

SourceLocation Patcher::findLocationForMove(NodeRef DstNode, NodeRef TargetNode,
                                            PatchedTreeNode &NewParent) {
  assert(isFromDst(DstNode));
  assert(isFromTarget(TargetNode));
  NodeRef DstParent = *DstNode.getParent();
  auto Range = DstNode.getSourceRange();
  SourceLocation MyLocInSource = Range.getBegin();
  TokenMapping MapDstTarget = matchTokens(DstParent, NewParent);
  BeforeThanCompare<SourceLocation> DstLess(Dst.getSourceManager());
  auto DstParentTokens = DstParent.getOwnedTokens();
  auto NewParentTokens = NewParent.getOwnedTokens();
  for (unsigned I = 0; I < NewParentTokens.size(); ++I) {
    int DstIndex = MapDstTarget.Mapping[I];
    if (DstIndex == -1)
      continue;
    SourceLocation NewLoc = NewParentTokens[I];
    SourceLocation DstLoc = DstParentTokens[DstIndex];
    if (wantToInsertBefore(MyLocInSource, DstLoc, DstLess)) {
      return NewLoc;
    }
  }
  SourceLocation Loc = NewParent.getSourceRange().getEnd();
  return Loc.isValid() ? Loc : SM.getLocForEndOfFile(SM.getMainFileID());
}

void Patcher::markChangedNodes() {
  for (auto Pair : InsertedNodes) {
    NodeRef DstNode = Dst.getNode(Pair.first);
    getPatchedNode(DstNode).Changed = true;
  }
  // Mark nodes in original as changed.
  for (NodeRef TargetNode : Target.postorder()) {
    auto &PatchedNode = PatchedTreeNodes[TargetNode.getId()];
    const Node *SrcNode = TargetDiff.getMapped(TargetNode);
    if (!SrcNode)
      continue;
    ChangeKind Change = Diff.getNodeChange(*SrcNode);
    auto &Children = PatchedNode.Children;
    bool AnyChildChanged =
        std::any_of(Children.begin(), Children.end(),
                    [](PatchedTreeNode *Child) { return Child->Changed; });
    bool AnyChildRemoved = std::any_of(
        PatchedNode.originalNode().begin(), PatchedNode.originalNode().end(),
        [this](NodeRef Child) { return isRemovedOrMoved(Child); });
    assert(!PatchedNode.Changed);
    PatchedNode.Changed =
        AnyChildChanged || AnyChildRemoved || Change != NoChange;
  }
}

Error Patcher::addReplacementsForChangedNodes() {
  for (NodeId TargetId = Target.getRootId(), E = Target.getSize(); TargetId < E;
       ++TargetId) {
    NodeRef TargetNode = Target.getNode(TargetId);
    auto &PatchedNode = getTargetPatchedNode(TargetNode);
    if (!PatchedNode.Changed)
      continue;
    if (TargetId == Target.getRootId())
      return addReplacementsForTopLevelChanges();
    CharSourceRange Range = getRangeForReplacing(TargetNode);
    std::string Text =
        isRemovedOrMoved(PatchedNode) ? "" : buildSourceText(PatchedNode);
    if (auto Err = addReplacement({SM, Range, Text, LangOpts}))
      return Err;
    TargetId = TargetNode.RightMostDescendant;
  }
  return Error::success();
}

Error Patcher::addReplacementsForTopLevelChanges() {
  auto &Root = getTargetPatchedNode(Target.getRoot());
  for (unsigned I = 0, E = Root.Children.size(); I < E; ++I) {
    PatchedTreeNode *Child = Root.Children[I];
    if (!Child->Changed)
      continue;
    std::string ChildText = buildSourceText(*Child);
    CharSourceRange ChildRange;
    if (isInserted(*Child) || isMoved(*Child)) {
      SourceLocation InsertionLoc;
      unsigned NumChildren = Root.Children.size();
      int ChildIndex;
      bool RightOfChild;
      std::tie(ChildIndex, RightOfChild) = findPointOfInsertion(*Child, Root);
      if (NumChildren && ChildIndex != -1) {
        auto NeighborRange = Root.Children[ChildIndex]->getSourceRange();
        InsertionLoc =
            RightOfChild ? NeighborRange.getEnd() : NeighborRange.getBegin();
      } else {
        InsertionLoc = SM.getLocForEndOfFile(SM.getMainFileID())
                           .getLocWithOffset(-int(strlen("\n")));
      }
      ChildRange = makeEmptyCharRange(InsertionLoc);
    } else {
      ChildRange = Child->getSourceRange();
    }
    if (auto Err = addReplacement({SM, ChildRange, ChildText, LangOpts})) {
      return Err;
    }
  }
  for (NodeRef Child : Root.originalNode()) {
    if (isRemovedOrMoved(Child)) {
      auto ChildRange = Child.findRangeForDeletion();
      if (auto Err = addReplacement({SM, ChildRange, "", LangOpts}))
        return Err;
    }
  }
  return Error::success();
}

static StringRef trailingText(SourceLocation Loc, SyntaxTree &Tree) {
  Token NextToken;
  bool Failure = Lexer::getRawToken(Loc, NextToken, Tree.getSourceManager(),
                                    Tree.getLangOpts(),
                                    /*IgnoreWhiteSpace=*/true);
  if (Failure)
    return StringRef();
  assert(!Failure);
  return Lexer::getSourceText(
      CharSourceRange::getCharRange({Loc, NextToken.getLocation()}),
      Tree.getSourceManager(), Tree.getLangOpts());
}

std::string Patcher::buildSourceText(PatchedTreeNode &PatchedNode) {
  auto &Children = PatchedNode.Children;
  auto &ChildrenOffsets = PatchedNode.ChildrenOffsets;
  auto &OwnText = PatchedNode.OwnText;
  auto Range = PatchedNode.getSourceRange();
  SyntaxTree &Tree = PatchedNode.getTree();
  SourceManager &MySM = Tree.getSourceManager();
  const LangOptions &MyLangOpts = Tree.getLangOpts();
  assert(!isRemoved(PatchedNode));
  if (!PatchedNode.Changed ||
      (isFromDst(PatchedNode) && AtomicInsertions[PatchedNode.getId()])) {
    std::string Text = Lexer::getSourceText(Range, MySM, MyLangOpts);
    // TODO why
    if (!isFromDst(PatchedNode))
      Text += trailingText(Range.getEnd(), Tree);
    return Text;
  }
  setOwnedSourceText(PatchedNode);
  std::string Result;
  unsigned Offset = 0;
  assert(ChildrenOffsets.size() == Children.size());
  for (unsigned I = 0, E = Children.size(); I < E; ++I) {
    PatchedTreeNode *Child = Children[I];
    unsigned Start = ChildrenOffsets[I];
    Result += OwnText->substr(Offset, Start - Offset);
    Result += buildSourceText(*Child);
    Offset = Start;
  }
  assert(Offset <= OwnText->size());
  Result += OwnText->substr(Offset, OwnText->size() - Offset);
  return Result;
}

StringRef getTokenText(SourceLocation Loc, SyntaxTree &Tree) {
  Token Tok;
  bool Failure =
      Lexer::getRawToken(Loc, Tok, Tree.getSourceManager(), Tree.getLangOpts(),
                         /*IgnoreWhiteSpace=*/true);
  assert(!Failure);
  auto TokenRange = CharSourceRange::getCharRange({Loc, Tok.getEndLoc()});
  return Lexer::getSourceText(TokenRange, Tree.getSourceManager(),
                              Tree.getLangOpts());
}

TokenMapping Patcher::matchTokens(NodeRef NodeA, NodeRef NodeB) {
  TokenMapping TheMapping;
  auto &TreeA = NodeA.getTree(), &TreeB = NodeB.getTree();
  auto A = NodeA.getOwnedTokens(), B = NodeB.getOwnedTokens();
  auto SizeA = A.size(), SizeB = B.size();
  std::vector<std::unique_ptr<int[]>> C(SizeA + 1);
  auto AreSameTokens = [&](SourceLocation LocA, SourceLocation LocB) {
    return getTokenText(LocA, TreeA) == getTokenText(LocB, TreeB);
  };
  for (auto &Row : C)
    Row = llvm::make_unique<int[]>(SizeB + 1);
  for (unsigned I = 0; I <= SizeA; ++I)
    C[I][0] = 0;
  for (unsigned J = 0; J <= SizeB; ++J)
    C[0][J] = 0;
  for (unsigned I = 1; I <= SizeA; ++I) {
    for (unsigned J = 1; J <= SizeB; ++J) {
      if (AreSameTokens(A[I - 1], B[J - 1]))
        C[I][J] = C[I - 1][J - 1] + 1;
      else
        C[I][J] = std::max(C[I][J - 1], C[I - 1][J]);
    }
  }
  auto FillUpMapping = [&](unsigned UpTo) {
    while (TheMapping.Mapping.size() < UpTo)
      TheMapping.Mapping.push_back(-1);
  };
  std::function<void(unsigned, unsigned)> BacktrackChanges = [&](unsigned I,
                                                                 unsigned J) {
    if (I > 0 && J > 0 && AreSameTokens(A[I - 1], B[J - 1])) {
      BacktrackChanges(I - 1, J - 1);
      FillUpMapping(J - 1);
      TheMapping.Mapping.push_back(I - 1);
    } else if (I > 0 && (J == 0 || C[I][J - 1] < C[I - 1][J])) {
      BacktrackChanges(I - 1, J);
      TheMapping.DeletedTokens.push_back(I - 1);
    } else if (J > 0 && (I == 0 || C[I][J - 1] >= C[I - 1][J])) {
      BacktrackChanges(I, J - 1);
      unsigned ChildIndex = 0;
      SourceLocation TokLoc = B[J - 1];
      BeforeThanCompare<SourceLocation> LessB(TreeB.getSourceManager());
      TokenInsertion Insertion;
      Insertion.Token = J - 1;
      if (I != 0) {
        Insertion.AfterToken = I - 1;
        for (NodeRef Child : NodeB) {
          SourceLocation ChildLoc = Child.getSourceRange().getBegin();
          if (LessB(TokLoc, ChildLoc) && ChildIndex) {
            Insertion.AfterChild = ChildIndex - 1;
            break;
          }
          ++ChildIndex;
        }
      }
      TheMapping.Insertions.emplace_back(Insertion);
    }
  };
  BacktrackChanges(SizeA, SizeB);
  FillUpMapping(SizeB);
  return TheMapping;
}

static void AppendToken(std::string &Text, SourceLocation Loc, SyntaxTree &Tree,
                        bool UnlessListSeparator = false) {
  Token Tok;
  bool Failure =
      Lexer::getRawToken(Loc, Tok, Tree.getSourceManager(), Tree.getLangOpts(),
                         /*IgnoreWhiteSpace=*/true);
  assert(!Failure);
  auto TokenRange = CharSourceRange::getCharRange({Loc, Tok.getEndLoc()});
  if (UnlessListSeparator && isListSeparator(Tok))
    return;
  Text += " ";
  Text += Lexer::getSourceText(TokenRange, Tree.getSourceManager(),
                               Tree.getLangOpts());
}

static bool haveCommonElement(ArrayRef<unsigned> A, ArrayRef<unsigned> B) {
  assert(std::is_sorted(A.begin(), A.end()) &&
         std::is_sorted(B.begin(), B.end()));
  for (unsigned I = 0, J = 0, SizeA = A.size(), SizeB = B.size();
       I < SizeA && J < SizeB;) {
    if (A[I] == B[J])
      return true;
    if (A[I] > B[J])
      ++J;
    else
      ++I;
  }
  return false;
}

static bool isInRange(SourceLocation Loc, CharSourceRange Range,
                      BeforeThanCompare<SourceLocation> &Less) {
  return !Less(Loc, Range.getBegin()) && !Less(Range.getEnd(), Loc);
};

void Patcher::setOwnedSourceText(PatchedTreeNode &PatchedNode) {
  assert(isFromTarget(PatchedNode) || isFromDst(PatchedNode));
  SyntaxTree &Tree = PatchedNode.getTree();
  const Node *SrcNode = nullptr;
  bool IsUpdate = false;
  auto &OwnText = PatchedNode.OwnText;
  auto &Children = PatchedNode.Children;
  auto &ChildrenLocations = PatchedNode.ChildrenLocations;
  auto &ChildrenOffsets = PatchedNode.ChildrenOffsets;
  OwnText = "";
  unsigned NumChildren = Children.size();
  if (isFromTarget(PatchedNode)) {
    SrcNode = TargetDiff.getMapped(PatchedNode);
    ChangeKind Change = SrcNode ? Diff.getNodeChange(*SrcNode) : NoChange;
    IsUpdate = Change == Update || Change == UpdateMove;
  }
  unsigned NumTokens;
  TokenMapping MapSrcDst, MapSrcTarget;
  auto Tokens = PatchedNode.getOwnedTokens();
  decltype(Tokens) DstTokens;
  if (IsUpdate) {
    assert(isFromTarget(PatchedNode));
    NodeRef DstNode = *Diff.getMapped(*SrcNode);
    DstTokens = DstNode.getOwnedTokens();
    MapSrcDst = matchTokens(*SrcNode, DstNode);
    MapSrcTarget = matchTokens(*SrcNode, PatchedNode);
    NumTokens = PatchedNode.getOwnedTokens().size();
  }
  unsigned NextChild = 0, NextToken = 0, NextTokenInSrc = 0,
           NextInsertedToken = 0;
  BeforeThanCompare<SourceLocation> MyLess(Tree.getSourceManager());
  auto AppendInsertedTokens = [&](int LastToken, int LastChild) {
    while (NextInsertedToken < MapSrcDst.Insertions.size()) {
      TokenInsertion &Insertion = MapSrcDst.Insertions[NextInsertedToken];
      if (Insertion.AfterChild > LastChild || Insertion.AfterToken > LastToken)
        break;
      AppendToken(*OwnText, DstTokens[Insertion.Token], Dst);
      *OwnText += " ";
      ++NextInsertedToken;
    }
  };
  auto AppendChildren = [&](CharSourceRange &InRange) {
    SourceLocation ChildBegin;
    while (NextChild < NumChildren &&
           ((ChildBegin = ChildrenLocations[NextChild]).isInvalid() ||
            wantToInsertBefore(ChildBegin, InRange.getEnd(), MyLess))) {
      ChildrenOffsets.push_back(OwnText->size());
      NodeRef Child = *Children[NextChild];
      const DynTypedNode &DTN = Child.ASTNode,
                         &ParentDTN = NodeRef(PatchedNode).ASTNode;
      if ((DTN.get<ParmVarDecl>() ||
           (ParentDTN.get<CallExpr>() && NextChild > 0)) &&
          NextChild + 1 < NumChildren) {
        *OwnText += ", ";
      }
      ++NextChild;
    }
  };
  auto DeletedBegin = MapSrcDst.DeletedTokens.begin();
  auto DeletedEnd = MapSrcDst.DeletedTokens.end();
  auto IsTokenDeleted = [&](unsigned TokenInSrc) {
    auto DeletedLoc = std::find(DeletedBegin, DeletedEnd, TokenInSrc);
    if (DeletedLoc != DeletedEnd) {
      DeletedBegin = DeletedLoc;
      return true;
    }
    return false;
  };
  auto AppendAndInsertTokens = [&](CharSourceRange &InRange) {
    SourceLocation TokenBegin;
    while (NextToken < NumTokens &&
           isInRange((TokenBegin = Tokens[NextToken]), InRange, Less)) {
      auto TokenIterator = std::find(MapSrcTarget.Mapping.begin(),
                                     MapSrcTarget.Mapping.end(), NextToken);
      bool IsTokenMapped = TokenIterator != MapSrcTarget.Mapping.end();
      if (IsTokenMapped)
        NextTokenInSrc = *TokenIterator;
      if (!IsTokenMapped || !IsTokenDeleted(NextTokenInSrc)) {
        AppendToken(*OwnText, TokenBegin, Target,
                    /*UnlessListSeparator=*/true);
      }
      if (IsTokenMapped) {
        AppendInsertedTokens(NextTokenInSrc, NextChild - 1);
      }
      ++NextToken;
    }
  };

  AppendInsertedTokens(TokenInsertion::BeforeFirst,
                       TokenInsertion::BeforeFirst);
  bool AnyTokenDeletedTwice =
      haveCommonElement(MapSrcDst.DeletedTokens, MapSrcTarget.DeletedTokens);
  IsUpdate = IsUpdate && !AnyTokenDeletedTwice;
  for (auto &MySubRange :
       PatchedNode.splitSourceRanges(PatchedNode.getOwnedSourceRanges())) {
    SourceLocation LastTokenLocation;
    AppendChildren(MySubRange);
    if (IsUpdate)
      AppendAndInsertTokens(MySubRange);
    else {
      forEachTokenInRange(
          MySubRange, Tree, [&Tree, &OwnText, &LastTokenLocation](Token &Tok) {
            AppendToken(*OwnText, (LastTokenLocation = Tok.getLocation()), Tree,
                        /*UnlessListSeparator=*/true);
          });
    }
  }
  while (NextChild++ < NumChildren) {
    ChildrenOffsets.push_back(OwnText->size());
    AppendInsertedTokens(NextTokenInSrc, NextChild - 1);
  }
}

std::pair<int, bool>
Patcher::findPointOfInsertion(NodeRef N, PatchedTreeNode &TargetParent) const {
  assert(isFromDst(N) || isFromTarget(N));
  assert(isFromTarget(TargetParent));
  auto MapFunction = [this, &N](PatchedTreeNode &Sibling) {
    if (isFromDst(N) == isFromDst(Sibling))
      return &NodeRef(Sibling);
    if (isFromDst(N))
      return mapTargetToDst(Sibling);
    else
      return mapDstToTarget(Sibling);
  };
  unsigned NumChildren = TargetParent.Children.size();
  BeforeThanCompare<SourceLocation> Less(N.getTree().getSourceManager());
  auto NodeIndex = N.findPositionInParent();
  SourceLocation MyLoc = N.getSourceRange().getBegin();
  assert(MyLoc.isValid());
  for (unsigned I = 0; I < NumChildren; ++I) {
    const Node *Sibling = MapFunction(*TargetParent.Children[I]);
    if (!Sibling)
      continue;
    SourceLocation SiblingLoc = Sibling->getSourceRange().getBegin();
    if (SiblingLoc.isInvalid())
      continue;
    if (NodeIndex && Sibling == &N.getParent()->getChild(NodeIndex - 1)) {
      return {I, /*RightOfSibling=*/true};
    }
    if (Less(MyLoc, SiblingLoc)) {
      return {I, /*RightOfSibling=*/false};
    }
  }
  return {-1, true};
}

static bool onlyWhitespace(StringRef Str) {
  return std::all_of(Str.begin(), Str.end(),
                     [](char C) { return std::isspace(C); });
}

SmallVector<CharSourceRange, 4>
PatchedTreeNode::splitSourceRanges(ArrayRef<CharSourceRange> SourceRanges) {
  SourceManager &SM = getTree().getSourceManager();
  const LangOptions &LangOpts = getTree().getLangOpts();
  SmallVector<CharSourceRange, 4> Result;
  BeforeThanCompare<SourceLocation> Less(SM);
  std::sort(SplitAt.begin(), SplitAt.end(), Less);
  for (auto &Range : SourceRanges) {
    SourceLocation Begin = Range.getBegin(), End = Range.getEnd();
    for (auto SplitPoint : SplitAt) {
      if (SM.isPointWithin(SplitPoint, Begin, End)) {
        auto SplitRange = CharSourceRange::getCharRange(Begin, SplitPoint);
        StringRef Text = Lexer::getSourceText(SplitRange, SM, LangOpts);
        if (onlyWhitespace(Text))
          continue;
        Result.emplace_back(SplitRange);
        Begin = SplitPoint;
      }
    }
    if (Less(Begin, End))
      Result.emplace_back(CharSourceRange::getCharRange(Begin, End));
  }
  return Result;
}

Error patch(RefactoringTool &TargetTool, SyntaxTree &Src, SyntaxTree &Dst,
            const ComparisonOptions &Options, bool Debug) {
  std::vector<std::unique_ptr<ASTUnit>> TargetASTs;
  TargetTool.buildASTs(TargetASTs);
  if (TargetASTs.size() != 1)
    return error(patching_error::failed_to_build_AST);
  SyntaxTree Target(*TargetASTs[0]);
  return Patcher(Src, Dst, Target, Options, TargetTool, Debug).apply();
}

std::string PatchingError::message() const {
  switch (Err) {
  case patching_error::failed_to_build_AST:
    return "Failed to build AST.\n";
  case patching_error::failed_to_apply_replacements:
    return "Failed to apply replacements.\n";
  case patching_error::failed_to_overwrite_files:
    return "Failed to overwrite some file(s).\n";
  };
}

char PatchingError::ID = 1;

} // end namespace diff
} // end namespace clang
