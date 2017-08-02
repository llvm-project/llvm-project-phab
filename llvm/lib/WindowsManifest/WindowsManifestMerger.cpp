//===-- WindowsManifestMerger.cpp ------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===//
//
// This file implements the .manifest merger class.
//
//===---------------------------------------------------------------------===//

#include "llvm/WindowsManifest/WindowsManifestMerger.h"
#include "llvm/Support/MemoryBuffer.h"

#include <stdarg.h>

#define TO_XML_CHAR(X) reinterpret_cast<const unsigned char *>(X)
#define FROM_XML_CHAR(X) reinterpret_cast<const char *>(X)

using namespace llvm;
using namespace windows_manifest;

char WindowsManifestError::ID = 0;

WindowsManifestError::WindowsManifestError(const Twine &Msg) : Msg(Msg.str()) {}

void WindowsManifestError::log(raw_ostream &OS) const { OS << Msg; }

#if LLVM_LIBXML2_ENABLED
static bool xmlStringsEqual(const unsigned char *A, const unsigned char *B) {
  if (!(A && B))
    return !A && !B;
  return strcmp(FROM_XML_CHAR(A), FROM_XML_CHAR(B)) == 0;
}
#endif

bool isMergeableElement(const unsigned char *ElementName) {
  for (StringRef S : {"application", "assembly", "assemblyIdentity",
                      "compatibility", "noInherit", "requestedExecutionLevel",
                      "requestedPrivileges", "security", "trustInfo"}) {
    if (S == FROM_XML_CHAR(ElementName))
      return true;
  }
  return false;
}

XMLNodeImpl getChildWithName(XMLNodeImpl Parent,
                             const unsigned char *ElementName) {
#if LLVM_LIBXML2_ENABLED
  for (XMLNodeImpl Child = Parent->children; Child; Child = Child->next)
    if (xmlStringsEqual(Child->name, ElementName)) {
      return Child;
    }
#endif
  return nullptr;
}

XMLAttrImpl getAttribute(XMLNodeImpl Node, const unsigned char *AttributeName) {
#if LLVM_LIBXML2_ENABLED
  for (xmlAttrPtr Attribute = Node->properties; Attribute != nullptr;
       Attribute = Attribute->next) {
    if (xmlStringsEqual(Attribute->name, AttributeName))
      return Attribute;
  }
#endif
  return nullptr;
}

bool namespaceOverrides(const unsigned char *Ns1, const unsigned char *Ns2) {
  auto Ns1Position = std::find_if(
      MtNsHrefsPrefixes.begin(), MtNsHrefsPrefixes.end(),
      [Ns1](const std::pair<std::string, std::string> &element) {
        return xmlStringsEqual(Ns1, TO_XML_CHAR(element.first.c_str()));
      });
  if (Ns1Position == MtNsHrefsPrefixes.end())
    return false;
  auto Ns2Position = std::find_if(
      MtNsHrefsPrefixes.begin(), MtNsHrefsPrefixes.end(),
      [Ns2](const std::pair<std::string, std::string> &element) {
        return xmlStringsEqual(Ns2, TO_XML_CHAR(element.first.c_str()));
      });
  if (Ns2Position == MtNsHrefsPrefixes.end())
    return true;
  int Ns1Priority = std::distance(Ns1Position, MtNsHrefsPrefixes.end());
  int Ns2Priority = std::distance(Ns2Position, MtNsHrefsPrefixes.end());
  if (Ns1Priority > Ns2Priority)
    return true;
  else
    return false;
}

XMLNsImpl search(const unsigned char *HRef, XMLNodeImpl Node) {
#if LLVM_LIBXML2_ENABLED
  // outs() << "searching for " << FROM_XML_CHAR(HRef) << " on " <<
  // FROM_XML_CHAR(Node->name) << "\n";
  for (XMLNsImpl Def = Node->nsDef; Def; Def = Def->next) {
    // outs() << "checking " << FROM_XML_CHAR(Def->prefix) << "\n";
    if (Def->prefix != nullptr && xmlStringsEqual(Def->href, HRef))
      return Def;
  }
  if (Node->parent) {
    return search(HRef, Node->parent);
  }
#endif
  return nullptr;
}

const unsigned char *getPrefixForHref(const unsigned char *HRef) {
  for (auto &Ns : MtNsHrefsPrefixes) {
    if (xmlStringsEqual(HRef, TO_XML_CHAR(Ns.first.c_str()))) {
      return TO_XML_CHAR(Ns.second.c_str());
    }
  }
  outs() << "could not find prefix\n";
  return HRef;
}

Expected<XMLNsImpl> searchOrDefine(const unsigned char *HRef,
                                   XMLNodeImpl Node) {
  // outs() << "search or define for " << FROM_XML_CHAR(HRef) << " on " <<
  // FROM_XML_CHAR(Node->name) << "\n";
  XMLNsImpl Def = search(HRef, Node);
  if (Def)
    return Def;
  Def = xmlNewNs(Node, HRef, getPrefixForHref(HRef));
  // outs() << "create namespace def with prefix " << FROM_XML_CHAR(Def->prefix)
  // << " on " << FROM_XML_CHAR(Node->name) << "\n";
  if (!Def)
    return make_error<WindowsManifestError>("failed to create new namespace");
  return Def;
}

Error copyAttributeNamespace(XMLAttrImpl OriginalAttribute,
                             XMLNodeImpl OriginalNode,
                             XMLAttrImpl AdditionalAttribute) {
  Expected<XMLNsImpl> ExplicitOrError =
      searchOrDefine(AdditionalAttribute->ns->href, OriginalNode);
  if (!ExplicitOrError)
    return ExplicitOrError.takeError();
  OriginalAttribute->ns = std::move(ExplicitOrError.get());
  return Error::success();
}

XMLNsImpl getNamespaceWithPrefix(const unsigned char *Prefix,
                                 XMLNodeImpl Node) {
#if LLVM_LIBXML2_ENABLED
  // outs() << "looking for namespace with prefix " << FROM_XML_CHAR(Prefix) <<
  // "\n";
  if (Node == nullptr)
    return nullptr;
  for (XMLNsImpl Def = Node->nsDef; Def; Def = Def->next) {
    if (xmlStringsEqual(Def->prefix, Prefix))
      return Def;
  }
#endif
  return nullptr;
}

XMLNsImpl getClosestDefault(XMLNodeImpl Node) {
#if LLVM_LIBXML2_ENABLED
  if (XMLNsImpl Ret = getNamespaceWithPrefix(nullptr, Node))
    return Ret;
  if (Node->parent == nullptr)
    return nullptr;
  return getClosestDefault(Node->parent);
#else
  return nullptr;
#endif
}

Error mergeAttributes(XMLNodeImpl OriginalNode, XMLNodeImpl AdditionalNode) {
#if LLVM_LIBXML2_ENABLED
  XMLNsImpl ClosestDefault = getClosestDefault(OriginalNode);
  for (xmlAttrPtr Attribute = AdditionalNode->properties; Attribute;
       Attribute = Attribute->next) {
    if (xmlAttrPtr OriginalAttribute =
            getAttribute(OriginalNode, Attribute->name)) {
      // outs() << "about to print ns\n";
      // outs() << FROM_XML_CHAR(Attribute->ns) << "\n";
      // outs() << "About to print namespace href\n";
      // outs() << FROM_XML_CHAR(Attribute->ns->href) << "\n";
      // outs() << "about to check namespace overrides\n";
      if (!xmlStringsEqual(OriginalAttribute->children->content,
                           Attribute->children->content))
        return make_error<WindowsManifestError>(
            Twine("conflicting attributes for ") +
            FROM_XML_CHAR(OriginalNode->name));

      // if (Attribute->ns && (!OriginalAttribute->ns ||
      // namespaceOverrides(Attribute->ns->href, OriginalAttribute->ns->href)))
      // {
      //   Expected<XMLNsImpl> ExplicitOrError =
      //   searchOrDefine(Attribute->ns->href, OriginalNode); if
      //   (!ExplicitOrError)
      //     return ExplicitOrError.takeError();
      //   XMLNsImpl Explicit = std::move(ExplicitOrError.get());
      //   OriginalAttribute->ns = Explicit;

      if (!Attribute->ns) {
        outs() << "no original namespace\n";
        continue;
      }
      if (!OriginalAttribute->ns) {
        outs() << "no attribute namespace\n";
        if (auto E = copyAttributeNamespace(OriginalAttribute, OriginalNode,
                                            Attribute))
          return E;
      } else if (namespaceOverrides(OriginalAttribute->ns->href,
                                    Attribute->ns->href)) {
        // outs() << "original namespace overrides\n";
        if (!OriginalAttribute->ns->prefix && !Attribute->ns->prefix &&
            ClosestDefault &&
            xmlStringsEqual(Attribute->ns->href, ClosestDefault->href)) {
          // outs() << "hit the case where we hvae to copy non-dominnat
          // namespace\n";
          if (auto E = copyAttributeNamespace(OriginalAttribute, OriginalNode,
                                              Attribute))
            return E;
        }
      } else if (Attribute->ns->prefix || OriginalAttribute->ns->prefix ||
                 (ClosestDefault &&
                  !xmlStringsEqual(OriginalAttribute->ns->href,
                                   ClosestDefault->href))) {
        outs() << "hitting the case where we have to copy the additional "
                  "namespace\n";
        outs() << "incoming prefix " << FROM_XML_CHAR(Attribute->ns->prefix)
               << "\n";
        outs() << "original prefix "
               << FROM_XML_CHAR(OriginalAttribute->ns->prefix) << "\n";
        outs() << "ClosestDefault " << FROM_XML_CHAR(ClosestDefault->href)
               << "\n";
        outs() << "incoming href " << FROM_XML_CHAR(Attribute->ns->href)
               << "\n";
        if (auto E = copyAttributeNamespace(OriginalAttribute, OriginalNode,
                                            Attribute))
          return E;
      }

      // if (OriginalAttribute->ns && (!Attribute->ns ||
      // namespaceOverrides(OriginalAttribute->ns->href, Attribute->ns->href)))
      // {
      //   if(!Attribute->ns  || (!OriginalAttribute->ns->prefix &&
      //   !Attribute->ns->prefix && xmlStringsEqual(ClosestDefault->ns->href, )
      // }
      // }
      // Attributes of the same name must also have the same value.  Otherwise
      // an error is thrown.
    } else {
      XMLAttrImpl NewProp = xmlNewProp(OriginalNode, Attribute->name,
                                       Attribute->children->content);
      Expected<XMLNsImpl> ExplicitOrError =
          searchOrDefine(Attribute->ns->href, OriginalNode);
      if (!ExplicitOrError)
        return ExplicitOrError.takeError();
      NewProp->ns = std::move(ExplicitOrError.get());
    }
  }
#endif
  return Error::success();
}

XMLNodeImpl getDominantNode(XMLNodeImpl Node1, XMLNodeImpl Node2) {
  if (!Node1 || !Node1->ns)
    return Node2;
  else if (!Node2 || !Node2->ns)
    return Node1;
  else if (namespaceOverrides(Node1->ns->href, Node2->ns->href))
    return Node1;
  else
    return Node2;
}

bool hasInheritedNs(XMLNodeImpl Node) {
#if LLVM_LIBXML2_ENABLED
  if (Node->ns && Node->ns != getNamespaceWithPrefix(Node->ns->prefix, Node))
    return true;
  else
#endif
    return false;
}

bool hasInheritedDefaultNs(XMLNodeImpl Node) {
#if LLVM_LIBXML2_ENABLED
  if (hasInheritedNs(Node) && Node->ns->prefix == nullptr)
    return true;
  else
#endif
    return false;
}

bool hasDefinedDefaultNamespace(XMLNodeImpl Node) {
#if LLVM_LIBXML2_ENABLED
  if (Node->ns && (Node->ns == getNamespaceWithPrefix(nullptr, Node)))
    return true;
  else
#endif
    return false;
}

void explicateNamespace(XMLNsImpl PrefixDef, XMLNodeImpl Node) {
#if LLVM_LIBXML2_ENABLED
  // outs() << "about to explicate on " << FROM_XML_CHAR(Node->name) << "\n";
  if (hasDefinedDefaultNamespace(Node))
    return;
  if (Node->ns && xmlStringsEqual(Node->ns->href, PrefixDef->href) &&
      hasInheritedDefaultNs(Node))
    Node->ns = PrefixDef;
  // for(xmlAttrPtr Attribute = Node->properties; Attribute; Attribute =
  // Attribute->next) {
  //   if (Attribute->ns && xmlStringsEqual(Attribute->ns->href,
  //   PrefixDef->href)) {
  //     Attribute->ns = PrefixDef;
  //   }
  // }
  for (XMLNodeImpl Child = Node->children; Child; Child = Child->next) {
    explicateNamespace(PrefixDef, Child);
  }
#endif
}

Error mergeNamespaces(XMLNodeImpl OriginalNode, XMLNodeImpl AdditionalNode) {
#if LLVM_LIBXML2_ENABLED
  const unsigned char *OriginalDefinedDefaultHref = nullptr;
  if (XMLNsImpl OriginalDefinedDefaultNs =
          getNamespaceWithPrefix(nullptr, OriginalNode)) {
    OriginalDefinedDefaultHref = xmlStrdup(OriginalDefinedDefaultNs->href);
    // outs() << "found defined default ns " <<
    // FROM_XML_CHAR(OriginalDefinedDefaultHref) << "\n";
  }
  for (XMLNsImpl Def = AdditionalNode->nsDef; Def; Def = Def->next) {
    // outs() << "merging namespace\n";
    if (XMLNsImpl OriginalNsDef =
            getNamespaceWithPrefix(Def->prefix, OriginalNode)) {
      // outs() << "found existing def with prefix\n";
      if (!Def->prefix) {
        if (namespaceOverrides(Def->href, OriginalNsDef->href)) {
          // outs() << "namespace overrides\n";
          xmlFree(const_cast<unsigned char *>(OriginalNsDef->href));
          OriginalNsDef->href = TO_XML_CHAR(strdup(FROM_XML_CHAR(Def->href)));
          // outs() << "def overrides so replace\n";
        }
      } else if (!xmlStringsEqual(OriginalNsDef->href, Def->href)) {
        // outs() << "conflicting namespace error\n";
        return make_error<WindowsManifestError>(
            Twine("conflicting namespace definitions for ") +
            FROM_XML_CHAR(Def->prefix));
      }
    } else {
      XMLNsImpl NewDef = xmlCopyNamespace(Def);
      NewDef->next = OriginalNode->nsDef;
      OriginalNode->nsDef = NewDef;
    }
  }
  // outs() << "Finished merging defs\n";
  XMLNodeImpl DominantNode = getDominantNode(OriginalNode, AdditionalNode);
  XMLNodeImpl NonDominantNode =
      DominantNode == OriginalNode ? AdditionalNode : OriginalNode;
  if (DominantNode == OriginalNode) {
    // outs() << "original dominant\n";
    if (OriginalDefinedDefaultHref) {
      XMLNsImpl NonDominantDefinedDefault =
          getNamespaceWithPrefix(nullptr, NonDominantNode);
      if (NonDominantDefinedDefault &&
          namespaceOverrides(NonDominantDefinedDefault->href,
                             OriginalDefinedDefaultHref)) {
        Expected<XMLNsImpl> EC =
            searchOrDefine(OriginalDefinedDefaultHref, DominantNode);
        if (!EC)
          return EC.takeError();
        XMLNsImpl PrefixDominantDefinedDefault = std::move(EC.get());
        explicateNamespace(PrefixDominantDefinedDefault, DominantNode);
      }
    } else if (getNamespaceWithPrefix(nullptr, NonDominantNode)) {
      // outs() << "found default namespace on NonDominantNode\n";
      if (DominantNode->parent) {
        XMLNsImpl ClosestDefault = getClosestDefault(DominantNode->parent);
        Expected<XMLNsImpl> EC =
            searchOrDefine(ClosestDefault->href, DominantNode);
        if (!EC)
          return EC.takeError();
        XMLNsImpl ExplicitDefault = std::move(EC.get());
        explicateNamespace(ExplicitDefault, DominantNode);
      }
    }
  } else {
    // outs() << "additional dominant\n";
    if (hasDefinedDefaultNamespace(DominantNode)) {
      NonDominantNode->ns = getNamespaceWithPrefix(nullptr, NonDominantNode);
    } else {
      Expected<XMLNsImpl> EC =
          searchOrDefine(DominantNode->ns->href, NonDominantNode);
      if (!EC)
        return EC.takeError();
      XMLNsImpl Explicit = std::move(EC.get());
      NonDominantNode->ns = Explicit;
      // outs() << "set explicit with prefix " <<
      // FROM_XML_CHAR(Explicit->prefix) << " on node " <<
      // FROM_XML_CHAR(NonDominantNode->name) << "\n";
    }
    if (XMLNsImpl DominantDefaultDefined =
            getNamespaceWithPrefix(nullptr, DominantNode)) {
      // outs() << "dominant defined default\n";
      if (OriginalDefinedDefaultHref) {
        if (namespaceOverrides(DominantDefaultDefined->href,
                               OriginalDefinedDefaultHref)) {
          Expected<XMLNsImpl> EC =
              searchOrDefine(OriginalDefinedDefaultHref, NonDominantNode);
          if (!EC)
            return EC.takeError();
          XMLNsImpl ExplicitDefault = std::move(EC.get());
          explicateNamespace(ExplicitDefault, NonDominantNode);
        }
      } else {
        XMLNsImpl ClosestDefault = getClosestDefault(NonDominantNode);
        Expected<XMLNsImpl> EC =
            searchOrDefine(ClosestDefault->href, NonDominantNode);
        if (!EC)
          return EC.takeError();
        XMLNsImpl ExplicitDefault = std::move(EC.get());
        explicateNamespace(ExplicitDefault, NonDominantNode);
      }
    }
  }
  xmlFree(const_cast<unsigned char *>(OriginalDefinedDefaultHref));
#endif
  return Error::success();
}

bool isRecognizedNamespace(const unsigned char *NsHref) {
  for (auto &Ns : MtNsHrefsPrefixes) {
    if (xmlStringsEqual(NsHref, TO_XML_CHAR(Ns.first.c_str())))
      return true;
  }
  return false;
}

bool hasRecognizedNamespace(XMLNodeImpl Node) {
#if LLVM_LIBXML2_ENABLED
  return isRecognizedNamespace(Node->ns->href);
#else
  return false;
#endif
}

Error reconcileNamespaces(XMLNodeImpl Node) {
#if LLVM_LIBXML2_ENABLED
  if (Node == nullptr)
    return Error::success();
  if (hasInheritedNs(Node)) {
    Expected<XMLNsImpl> ExplicitOrError = searchOrDefine(Node->ns->href, Node);
    if (!ExplicitOrError)
      return ExplicitOrError.takeError();
    XMLNsImpl Explicit = std::move(ExplicitOrError.get());
    Node->ns = Explicit;
  }
  for (XMLNodeImpl Child = Node->children; Child; Child = Child->next) {
    if (auto E = reconcileNamespaces(Child))
      return E;
  }
#endif
  return Error::success();
}

// Error mergeNamespaces(XMLNodeImpl OriginalNode, XMLNodeImpl AdditionalNode) {
//   #if LLVM_LIBXML2_ENABLED
//   for (XMLNsImpl Def = AdditionalNode->nsDef; Def; Def = Def->next) {
//     if (XMLNsImpl OriginalNsDef = getNamespaceWithPrefix(Def->prefix,
//     OriginalNode)) {
//       if (Def->prefix == nullptr && namespaceOverrides(Def, OriginalNsDef)) {
//         xmlFree(OriginalDefinedDefaultNs->href);
//         OriginalDefinedDefaultNs->href =
//         TO_XML_CHAR(strdup(FROM_XML_CHAR(Def->href)));
//       } else if (!xmlStringsEqual(OriginalNsDef->href, Def->href)) {
//         return make_error<WindowsManifestError>(Twine("conflicting namespace
//         definitions for ") + FROM_XML_CHAR(Def->prefix));
//       }
//     } else {
//       XMLNsImpl NewDef = xmlCopyNamespace(Def);
//       NewDef->next = OriginalNode->nsDef;
//       OriginalNode->nsDef = NewDef;
//     }
//   }

//   XMLNodeImpl DominantNode = getDominantNode(OriginalNode, AdditionalNode);
//   XMLNodeImpl NonDominantNode = DominantNode == OriginalNode ? AdditionalNode
//   : DominantNode; if (hasInheritedDefaultNs(NonDominantNode)) {
//     XMLNsImpl NewDef = searchOrAddNsDef(NonDominantNode,
//     NonDominantNode->ns); propagateNamespace(NonDominantNode, NewDef);
//   }
//   if (hasDefinedDefaultNs(DominantNode) && DominantNode != OriginalNode) {
//     XMLNsImpl NewDef = addNsDef(OriginalNode, DominantNode->ns->href,
//     nullptr); OriginalNode->ns = NewDef;
//     if(defaultNamespaceDefinedOn(OriginalNode))
//       propagateNamespace(NewDef);
//   } else if (hasInheritedNs(DominantNode) &&
//   (defaultNamespaceDefinedOn(NonDominantNode) || NonDominantNode ==
//   OriginalNode)) {
//     XMLNsImpl NewNs = searchOrAddNsDef(OriginalNode, DominantNode->ns);
//     OriginalNode->ns = newNs;
//     if (hasInheritedDefaultNs(DominantNode)) {
//       XMLNsImpl NewDef = searchOrAddNsDef(DominantNode, DominantNode->ns);
//       propagateNamespace(DominantNode, NewDef);
//     }
//   }
//   #endif
// }

// XMLNodeImpl DominantNode = getDominantNode(OriginalNode, AdditionalNode);
// OriginalNode->ns = DominantNode->ns;
// if (isInheritedNamespace(DominantNode->ns) && OriginalNode != DominantNode) {
//   if (isDefaultNamespace(DominantNode->ns)) {
//     XMLNsImpl NewNs = defineNamespace(OriginalNode, DominantNode->ns);
//     OriginalNode->ns = NewNs;
//     explicateNamespace(DominantNode, NewNs);
//   } else {
//     if (XMLNsImpl PreviousDef = searchAncestorsForNsDef(OriginalNode,
//     DominantNode->ns->ref)) {
//       OriginalNode->ns = PreviousDef;
//     } else {
//       XMLNsImpl NewNs = defineNamespace(OriginalNode, DominantNode->ns);
//       OriginalNode->ns = NewNs;
//     }
//   }
// }
// XMLNodeImpl NonDominantNode = OriginalNode == DominantNode ? AdditionalNode :
// OriginalNode; if (isInheritedNamespace(NonDominantNode->ns) &&
// isDefaultNamespace(NonDominantNode->ns)) {
//   XMLNsImpl OldNs = defineNamespace(OriginalNode, NonDominantNode->ns);
//   explicateNamespace(NonDominantNode, OldNs);
// }

// void mergeNamespaces(XMLNodeImpl OriginalNode, XMLNodeImpl AdditionalNode) {
//   #if LLVM_LIBXML2_ENABLED
//   XMLNodeImpl DominantNode = getDominantNode(OriginalNode, AdditionalNode);
//   XMLNodeImpl NonDominantNode = DominantNode == OriginalNode ? AdditionalNode
//   : OriginalNode; if (isDefaultNamespace(NonDominantNode->ns,
//   NonDominantNode)) {
//     XMLNsImpl NonDominantNs = xmlNewNs(NonDominantNode,
//     NonDominantNode->ns->href,
//     getCorrespondingPrefix(NonDominantNode->ns->href));
//     explicateNamespace(NonDominantNode, NonDominantNs);
//     if (DominantNode == OriginalNode) return;
//   }
//   if (isDefaultNamespace(DominantNode->ns, DominantNode)) {
//     unsigned char *Prefix = getCorrespondingPrefix(DominantNs->href);
//     DominantNode->ns = xmlNewNs(DominantNode, DominantNs->href, Prefix);
//     explicateNamespace(DominantNode, DominantNode->ns);
//     if (OriginalNode != DominantNode)
//       OriginalNode->ns = xmlNewNs(OriginalNode, DominantNs->href, Prefix);
//   } else {
//     xmlFree(OriginalNode->ns->href);
//     OriginalNode->ns->href =
//     TO_XML_CHAR(strdup(FROM_XML_CHAR(DominantNs->href)));
//     OriginalNode->ns->prefix =
//     TO_XML_CHAR(strdup(FROM_XML_CHAR(DominantNs->prefix)));
//   }
//   #endif
// }

// Recursively merge the two given manifest trees, depending on which elements
// are of a mergeable type, and choose namespaces according to which have
// higher priority.
Error treeMerge(XMLNodeImpl OriginalRoot, XMLNodeImpl AdditionalRoot) {

  outs() << "merging node " << FROM_XML_CHAR(AdditionalRoot->name) << "\n";
#if LLVM_LIBXML2_ENABLED
  if (auto E = mergeAttributes(OriginalRoot, AdditionalRoot))
    return E;
  if (auto E = mergeNamespaces(OriginalRoot, AdditionalRoot))
    return E;
  XMLNodeImpl AdditionalFirstChild = AdditionalRoot->children;
  xmlNode StoreNext;
  for (XMLNodeImpl Child = AdditionalFirstChild; Child; Child = Child->next) {
    XMLNodeImpl OriginalChildWithName;
    if (!isMergeableElement(Child->name) ||
        !(OriginalChildWithName =
              getChildWithName(OriginalRoot, Child->name)) ||
        !hasRecognizedNamespace(Child)) {
      StoreNext.next = Child->next;
      xmlUnlinkNode(Child);
      if (!xmlAddChild(OriginalRoot, Child))
        return make_error<WindowsManifestError>(Twine("could not merge ") +
                                                FROM_XML_CHAR(Child->name));
      if (auto E = reconcileNamespaces(Child))
        return E;
      Child = &StoreNext;
    } else if (auto E = treeMerge(OriginalChildWithName, Child)) {
      return E;
    }
  }
#endif

  outs() << "finished merging node " << FROM_XML_CHAR(AdditionalRoot->name)
         << "\n";
  return Error::success();
}

void stripComments(XMLNodeImpl Root) {
#if LLVM_LIBXML2_ENABLED
  xmlNode StoreNext;
  for (XMLNodeImpl Child = Root->children; Child; Child = Child->next) {
    if (!xmlStringsEqual(Child->name, TO_XML_CHAR("comment"))) {
      stripComments(Child);
    } else {
      StoreNext.next = Child->next;
      XMLNodeImpl Remove = Child;
      Child = &StoreNext;
      xmlUnlinkNode(Remove);
      xmlFreeNode(Remove);
    }
  }
#endif
}

WindowsManifestMerger::~WindowsManifestMerger() {
#if LLVM_LIBXML2_ENABLED
  for (auto &Doc : MergedDocs)
    xmlFreeDoc(Doc);
#endif
}

void setAttributeNamespaces(XMLNodeImpl Node) {
  for (XMLAttrImpl Attribute = Node->properties; Attribute;
       Attribute = Attribute->next) {
    if (!Attribute->ns) {
      Attribute->ns = getClosestDefault(Node);
    }
  }
  for (XMLNodeImpl Child = Node->children; Child; Child = Child->next) {
    setAttributeNamespaces(Child);
  }
}

Error WindowsManifestMerger::merge(const MemoryBuffer &Manifest) {
#if LLVM_LIBXML2_ENABLED
  if (Manifest.getBufferSize() == 0)
    return make_error<WindowsManifestError>(
        "attempted to merge empty manifest");
  xmlSetGenericErrorFunc((void *)this, WindowsManifestMerger::errorCallback);
  XMLDocumentImpl ManifestXML =
      xmlReadMemory(Manifest.getBufferStart(), Manifest.getBufferSize(),
                    "manifest.xml", nullptr, XML_PARSE_NOBLANKS);
  xmlSetGenericErrorFunc(nullptr, nullptr);
  if (auto E = getParseError())
    return E;
  XMLNodeImpl AdditionalRoot = xmlDocGetRootElement(ManifestXML);
  stripComments(AdditionalRoot);
  setAttributeNamespaces(AdditionalRoot);
  if (CombinedDoc == nullptr) {
    CombinedDoc = ManifestXML;
  } else {
    XMLNodeImpl CombinedRoot = xmlDocGetRootElement(CombinedDoc);
    if (xmlStringsEqual(CombinedRoot->name, AdditionalRoot->name) &&
        isMergeableElement(AdditionalRoot->name) &&
        hasRecognizedNamespace(AdditionalRoot)) {
      if (auto E = treeMerge(CombinedRoot, AdditionalRoot)) {
        return E;
      }
    } else {
      return make_error<WindowsManifestError>("multiple root nodes");
    }
  }
  MergedDocs.push_back(ManifestXML);
#endif
  // outs() << "finished merge\n";
  return Error::success();
}

void checkAndStripPrefixes(XMLNodeImpl Node,
                           std::vector<XMLNsImpl> &RequiredPrefixes) {
#if LLVM_LIBXML2_ENABLED
  for (XMLNodeImpl Child = Node->children; Child; Child = Child->next) {
    checkAndStripPrefixes(Child, RequiredPrefixes);
  }
  if (Node->ns && Node->ns->prefix != nullptr) {
    // outs() << "about to get closest default\n";
    XMLNsImpl ClosestDefault = getClosestDefault(Node);
    // outs() << "got closest default\n";
    if (ClosestDefault &&
        xmlStringsEqual(ClosestDefault->href, Node->ns->href)) {
      Node->ns = ClosestDefault;
    } else if (std::find(RequiredPrefixes.begin(), RequiredPrefixes.end(),
                         Node->ns) == RequiredPrefixes.end()) {
      RequiredPrefixes.push_back(Node->ns);
    }
  }
  for (XMLAttrImpl Attribute = Node->properties; Attribute;
       Attribute = Attribute->next) {
    if (Attribute->ns && Attribute->ns->prefix != nullptr) {
      // outs() << "about to get closest default\n";
      XMLNsImpl ClosestDefault = getClosestDefault(Node);
      // outs() << "got closest default\n";
      if (ClosestDefault &&
          xmlStringsEqual(ClosestDefault->href, Attribute->ns->href)) {
        Attribute->ns = ClosestDefault;
      } else if (std::find(RequiredPrefixes.begin(), RequiredPrefixes.end(),
                           Node->ns) == RequiredPrefixes.end()) {
        RequiredPrefixes.push_back(Attribute->ns);
      }
    }
  }
  XMLNsImpl Prev;
  xmlNs Temp;
  for (XMLNsImpl Def = Node->nsDef; Def; Def = Def->next) {
    if (Def->prefix &&
        (std::find(RequiredPrefixes.begin(), RequiredPrefixes.end(), Def) ==
         RequiredPrefixes.end())) {
      if (Def == Node->nsDef)
        Node->nsDef = Def->next;
      else
        Prev->next = Def->next;
      Temp.next = Def->next;
      // outs() << "about to free ns\n";
      xmlFreeNs(Def);
      Def = &Temp;
      // outs() << "freed ns\n";
    } else {
      Prev = Def;
    }
  }
  // outs() << "returned check and strip\n";
#endif
}

std::unique_ptr<MemoryBuffer> WindowsManifestMerger::getMergedManifest() {
// outs() << "getting merged manifest\n";
#if LLVM_LIBXML2_ENABLED
  unsigned char *XmlBuff;
  int BufferSize = 0;
  if (CombinedDoc) {
    CombinedDoc->standalone = 1;
    XMLNodeImpl CombinedRoot = xmlDocGetRootElement(CombinedDoc);
    std::vector<XMLNsImpl> RequiredPrefixes;
    // outs() << "about to check and strip\n";
    checkAndStripPrefixes(CombinedRoot, RequiredPrefixes);
    // outs() << "finished checking and stripping\n";
    std::unique_ptr<xmlDoc> OutputDoc(xmlNewDoc((const unsigned char *)"1.0"));
    xmlDocSetRootElement(OutputDoc.get(), CombinedRoot);
    xmlKeepBlanksDefault(0);
    xmlDocDumpFormatMemoryEnc(OutputDoc.get(), &XmlBuff, &BufferSize, "UTF-8",
                              1);
  }
  if (BufferSize == 0)
    return nullptr;
  // outs() << "about to return membuffer\n";
  return MemoryBuffer::getMemBuffer(
      StringRef(FROM_XML_CHAR(XmlBuff), (size_t)BufferSize));
#else
  return nullptr;
#endif
}

void WindowsManifestMerger::errorCallback(void *Ctx, const char *Format, ...) {
  auto *Merger = (WindowsManifestMerger *)Ctx;
  Merger->ParseErrorOccurred = true;
}

Error WindowsManifestMerger::getParseError() {
  if (!ParseErrorOccurred)
    return Error::success();
  return make_error<WindowsManifestError>("invalid xml document");
}
