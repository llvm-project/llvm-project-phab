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

static const std::pair<std::string, std::string>
    MtNsHrefsPrefixes[] = {
        {"urn:schemas-microsoft-com:asm.v1", "ms_asmv1"},
        {"urn:schemas-microsoft-com:asm.v2", "ms_asmv2"},
        {"urn:schemas-microsoft-com:asm.v3", "ms_asmv3"},
        {"http://schemas.microsoft.com/SMI/2005/WindowsSettings",
         "ms_windowsSettings"},
        {"urn:schemas-microsoft-com:compatibility.v1", "ms_compatibilityv1"}};

static bool xmlStringsEqual(const unsigned char *A, const unsigned char *B) {
  // Handle null pointers.  Comparison of 2 null pointers returns true because
  // this indicates the prefix of a default namespace.
  if (!A || !B)
    return A == B;

  return strcmp(FROM_XML_CHAR(A), FROM_XML_CHAR(B)) == 0;
}

static bool isMergeableElement(const unsigned char *ElementName) {
  for (StringRef S : {"application", "assembly", "assemblyIdentity",
                      "compatibility", "noInherit", "requestedExecutionLevel",
                      "requestedPrivileges", "security", "trustInfo"}) {
    if (S == FROM_XML_CHAR(ElementName))
      return true;
  }
  return false;
}

static XMLNodeImpl getChildWithName(XMLNodeImpl Parent,
                             const unsigned char *ElementName) {
  for (XMLNodeImpl Child = Parent->children; Child; Child = Child->next)
    if (xmlStringsEqual(Child->name, ElementName)) {
      return Child;
    }
  return nullptr;
}

static XMLAttrImpl getAttribute(XMLNodeImpl Node, const unsigned char *AttributeName) {
  for (xmlAttrPtr Attribute = Node->properties; Attribute != nullptr;
       Attribute = Attribute->next) {
    if (xmlStringsEqual(Attribute->name, AttributeName))
      return Attribute;
  }
  return nullptr;
}

// Check if namespace specified by HRef1 overrides that of HRef2.
static bool namespaceOverrides(const unsigned char *HRef1,
                        const unsigned char *HRef2) {
  auto HRef1Position = std::find_if(
      std::begin(MtNsHrefsPrefixes), std::end(MtNsHrefsPrefixes),
      [HRef1](const std::pair<std::string, std::string> &element) {
        return xmlStringsEqual(HRef1, TO_XML_CHAR(element.first.c_str()));
      });
  if (HRef1Position == std::end(MtNsHrefsPrefixes))
    return false;
  auto HRef2Position = std::find_if(
      std::begin(MtNsHrefsPrefixes), std::end(MtNsHrefsPrefixes),
      [HRef2](const std::pair<std::string, std::string> &element) {
        return xmlStringsEqual(HRef2, TO_XML_CHAR(element.first.c_str()));
      });
  if (HRef2Position == std::end(MtNsHrefsPrefixes))
    return true;
  int HRef1Priority = std::distance(HRef1Position, std::end(MtNsHrefsPrefixes));
  int HRef2Priority = std::distance(HRef2Position, std::end(MtNsHrefsPrefixes));
  if (HRef1Priority > HRef2Priority)
    return true;
  return false;
}

// Search for prefix-defined namespace specified by HRef, starting on Node and
// continuing recurisvely upwards.  Returns the namespace or nullptr if not
// found.
static XMLNsImpl search(const unsigned char *HRef, XMLNodeImpl Node) {
  for (XMLNsImpl Def = Node->nsDef; Def; Def = Def->next) {
    if (Def->prefix && xmlStringsEqual(Def->href, HRef))
      return Def;
  }
  if (Node->parent) {
    return search(HRef, Node->parent);
  }
  return nullptr;
}

// Return the prefix that corresponds to the HRef.  If HRef is not a recognized
// URI, then just return the HRef itself to use as the prefix.
static const unsigned char *getPrefixForHref(const unsigned char *HRef) {
  for (auto &Ns : MtNsHrefsPrefixes) {
    if (xmlStringsEqual(HRef, TO_XML_CHAR(Ns.first.c_str()))) {
      return TO_XML_CHAR(Ns.second.c_str());
    }
  }
  return HRef;
}

// Search for prefix-defined namespace specified by HRef, starting on Node and
// continuing recurisvely upwards.  If it is found, then return it.  If it is
// not found, then prefix-define that namespace on the node and return a
// reference to it.
static Expected<XMLNsImpl> searchOrDefine(const unsigned char *HRef,
                                   XMLNodeImpl Node) {
  XMLNsImpl Def = search(HRef, Node);
  if (Def)
    return Def;
  Def = xmlNewNs(Node, HRef, getPrefixForHref(HRef));
  if (!Def)
    return make_error<WindowsManifestError>("failed to create new namespace");
  return Def;
}

// Set the namespace of OrigionalAttribute on OriginalNode to be that of
// AdditionalAttribute's.
static Error copyAttributeNamespace(XMLAttrImpl OriginalAttribute,
                             XMLNodeImpl OriginalNode,
                             XMLAttrImpl AdditionalAttribute) {

  Expected<XMLNsImpl> ExplicitOrError =
      searchOrDefine(AdditionalAttribute->ns->href, OriginalNode);
  if (!ExplicitOrError)
    return ExplicitOrError.takeError();
  OriginalAttribute->ns = std::move(ExplicitOrError.get());
  return Error::success();
}

// Return the corresponding namespace definition for the prefix, defined on the
// given Node.  Returns nullptr if there is no such definition.
static XMLNsImpl getNamespaceWithPrefix(const unsigned char *Prefix,
                                 XMLNodeImpl Node) {

  if (Node == nullptr)
    return nullptr;
  for (XMLNsImpl Def = Node->nsDef; Def; Def = Def->next) {
    if (xmlStringsEqual(Def->prefix, Prefix))
      return Def;
  }
  return nullptr;
}

// Search for the closest inheritable default namespace, starting on (and
// including) the Node and traveling upwards through parent nodes.  Returns
// nullptr if there are no inheritable default namespaces.
static XMLNsImpl getClosestDefault(XMLNodeImpl Node) {
  if (XMLNsImpl Ret = getNamespaceWithPrefix(nullptr, Node))
    return Ret;
  if (Node->parent == nullptr)
    return nullptr;
  return getClosestDefault(Node->parent);
}

// Merge the attributes of AdditionalNode into OriginalNode.  If attributes
// with identical types are present, they are not duplicated but rather if
// their values are not consistent and error is thrown.  In addition, the
// higher priority namespace is used for each attribute, EXCEPT in the case
// of merging two default namespaces and the lower priority namespace
// definition occurs closer than the higher priority one.
static Error mergeAttributes(XMLNodeImpl OriginalNode, XMLNodeImpl AdditionalNode) {
  XMLNsImpl ClosestDefault = getClosestDefault(OriginalNode);
  for (xmlAttrPtr Attribute = AdditionalNode->properties; Attribute;
       Attribute = Attribute->next) {
    if (xmlAttrPtr OriginalAttribute =
            getAttribute(OriginalNode, Attribute->name)) {
      if (!xmlStringsEqual(OriginalAttribute->children->content,
                           Attribute->children->content))
        return make_error<WindowsManifestError>(
            Twine("conflicting attributes for ") +
            FROM_XML_CHAR(OriginalNode->name));
      if (!Attribute->ns) {
        continue;
      }
      if (!OriginalAttribute->ns) {
        if (auto E = copyAttributeNamespace(OriginalAttribute, OriginalNode,
                                            Attribute))
          return E;
        // In this case, the original attribute has a higher priority namespace
        // than the incomiing attribute, however the namespace definition of
        // the lower priority namespace occurs first traveling upwards in the
        // tree.  Therefore the lower priority namespace is applied.
      } else if (namespaceOverrides(OriginalAttribute->ns->href,
                                    Attribute->ns->href)) {
        if (!OriginalAttribute->ns->prefix && !Attribute->ns->prefix &&
            ClosestDefault &&
            xmlStringsEqual(Attribute->ns->href, ClosestDefault->href)) {
          if (auto E = copyAttributeNamespace(OriginalAttribute, OriginalNode,
                                              Attribute))
            return E;
        }
        // This covers the case where the incoming attribute has the higher
        // priority.  The higher priority namespace is applied in all cases
        // EXCEPT when both of the namespaces are default inherited, and the
        // closest inherited default is the lower priority one.
      } else if (Attribute->ns->prefix || OriginalAttribute->ns->prefix ||
                 (ClosestDefault &&
                  !xmlStringsEqual(OriginalAttribute->ns->href,
                                   ClosestDefault->href))) {
        if (auto E = copyAttributeNamespace(OriginalAttribute, OriginalNode,
                                            Attribute))
          return E;
      }
      // If the incoming attribute is not already found on the node, append it
      // to the end of the properties list.  Also explicitly apply its
      // namespace as a prefix because it might be contained in a separate
      // namespace that doesn't use the attribute.
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
  return Error::success();
}

// Given two nodes, return the one with the higher priority namespace.
static XMLNodeImpl getDominantNode(XMLNodeImpl Node1, XMLNodeImpl Node2) {

  if (!Node1 || !Node1->ns)
    return Node2;
  else if (!Node2 || !Node2->ns)
    return Node1;
  else if (namespaceOverrides(Node1->ns->href, Node2->ns->href))
    return Node1;
  else
    return Node2;
}

// Checks if this Node's namespace is inherited or one it defined itself.
static bool hasInheritedNs(XMLNodeImpl Node) {
  if (Node->ns && Node->ns != getNamespaceWithPrefix(Node->ns->prefix, Node))
    return true;
  else
    return false;
}

// Check if this Node's namespace is a default namespace that it inherited, as
// opposed to defining itself.
static bool hasInheritedDefaultNs(XMLNodeImpl Node) {
  if (hasInheritedNs(Node) && Node->ns->prefix == nullptr)
    return true;
  else
    return false;
}

// Check if this Node's namespace is a default namespace it defined itself.
static bool hasDefinedDefaultNamespace(XMLNodeImpl Node) {
  if (Node->ns && (Node->ns == getNamespaceWithPrefix(nullptr, Node)))
    return true;
  else
    return false;
}

// For the given explicit prefix-definition of a namespace, travel downwards
// from a node recursively, and for every implicit, inherited default usage of
// that namespace replace it with that explicit prefix use.  This is important
// when namespace overriding occurs when merging, so that elements unique to a
// namespace will still stay in that namespace.
static void explicateNamespace(XMLNsImpl PrefixDef, XMLNodeImpl Node) {
  // If a node as its own default namespace definition it clearly cannot have
  // inherited the given default namespace, and neither will any of its
  // children.
  if (hasDefinedDefaultNamespace(Node))
    return;
  if (Node->ns && xmlStringsEqual(Node->ns->href, PrefixDef->href) &&
      hasInheritedDefaultNs(Node))
    Node->ns = PrefixDef;
  for (xmlAttrPtr Attribute = Node->properties; Attribute;
       Attribute = Attribute->next) {
    if (Attribute->ns &&
        xmlStringsEqual(Attribute->ns->href, PrefixDef->href)) {
      Attribute->ns = PrefixDef;
    }
  }
  for (XMLNodeImpl Child = Node->children; Child; Child = Child->next) {
    explicateNamespace(PrefixDef, Child);
  }
}

// Perform the namespace merge between two nodes.
static Error mergeNamespaces(XMLNodeImpl OriginalNode, XMLNodeImpl AdditionalNode) {
  // Save the original default namespace definition in case the incoming node
  // overrides it.
  const unsigned char *OriginalDefinedDefaultHref = nullptr;
  if (XMLNsImpl OriginalDefinedDefaultNs =
          getNamespaceWithPrefix(nullptr, OriginalNode)) {
    OriginalDefinedDefaultHref = xmlStrdup(OriginalDefinedDefaultNs->href);
  }
  const unsigned char *NewDefinedDefaultHref = nullptr;
  // Copy all namespace definitions.  There can only be one default namespace
  // definition per node, so the higher priority one takes precedence in the
  // case of collision.
  for (XMLNsImpl Def = AdditionalNode->nsDef; Def; Def = Def->next) {
    if (XMLNsImpl OriginalNsDef =
            getNamespaceWithPrefix(Def->prefix, OriginalNode)) {
      if (!Def->prefix) {
        if (namespaceOverrides(Def->href, OriginalNsDef->href)) {
          NewDefinedDefaultHref = TO_XML_CHAR(strdup(FROM_XML_CHAR(Def->href)));
        }
      } else if (!xmlStringsEqual(OriginalNsDef->href, Def->href)) {
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

  // Check whether the original node or the incoming node has the higher
  // priority namespace.  Depending on which one is dominant, we will have
  // to recursively apply namespace changes down to children of the original
  // node.
  XMLNodeImpl DominantNode = getDominantNode(OriginalNode, AdditionalNode);
  XMLNodeImpl NonDominantNode =
      DominantNode == OriginalNode ? AdditionalNode : OriginalNode;
  if (DominantNode == OriginalNode) {
    if (OriginalDefinedDefaultHref) {
      XMLNsImpl NonDominantDefinedDefault =
          getNamespaceWithPrefix(nullptr, NonDominantNode);
      // In this case, both the nodes defined a default namespace.  However
      // the lower priority node ended up having a higher priority default
      // definition.  This can occur if the higher priority node is prefix
      // namespace defined.  In this case we have to define an explicit
      // prefix for the overridden definition and apply it to all children
      // who relied on that definition.
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
      // In this case the node with a higher priority namespace did not have a
      // default namespace definition, but the lower priority node did.  In this
      // case the new default namespace definition is copied.  A side effect of
      // this is that all children will suddenly find themselves in a different
      // default namespace.  To maintain correctness we need to ensure that all
      // children now explicitly refer to the namespace that they had previously
      // implicitly inherited.
    } else if (getNamespaceWithPrefix(nullptr, NonDominantNode)) {
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
    // Covers case where the incoming node has a default namespace definition
    // that overrides the original node's namespace.  This always leads to
    // the original node receiving that new default namespace.
    if (hasDefinedDefaultNamespace(DominantNode)) {
      NonDominantNode->ns = getNamespaceWithPrefix(nullptr, NonDominantNode);
    } else {
      // This covers the case where the incoming node either has a prefix
      // namespace, or an inherited default namespace.  Since the namespace
      // may not yet be defined in the original tree we do a searchOrDefine
      // for it, and then set the namespace equal to it.
      Expected<XMLNsImpl> EC =
          searchOrDefine(DominantNode->ns->href, NonDominantNode);
      if (!EC)
        return EC.takeError();
      XMLNsImpl Explicit = std::move(EC.get());
      NonDominantNode->ns = Explicit;
    }
    // This covers cases where the incoming dominant node HAS a default
    // namespace definition, but MIGHT NOT NECESSARILY be in that namespace.
    if (XMLNsImpl DominantDefaultDefined =
            getNamespaceWithPrefix(nullptr, DominantNode)) {
      if (OriginalDefinedDefaultHref) {
        if (namespaceOverrides(DominantDefaultDefined->href,
                               OriginalDefinedDefaultHref)) {
          // In this case, the incoming node's default definition overrides
          // the original default definition, all children who relied on that
          // definition must be updated accordingly.
          Expected<XMLNsImpl> EC =
              searchOrDefine(OriginalDefinedDefaultHref, NonDominantNode);
          if (!EC)
            return EC.takeError();
          XMLNsImpl ExplicitDefault = std::move(EC.get());
          explicateNamespace(ExplicitDefault, NonDominantNode);
        }
      } else {
        // The original did not define a default definition, however the new
        // default definition still applies to all children, so they must be
        // updated to explicitly refer to the namespace they had previously
        // been inheriting implicitly.
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
  if (NewDefinedDefaultHref) {
    XMLNsImpl OriginalNsDef = getNamespaceWithPrefix(nullptr, OriginalNode);
    xmlFree(const_cast<unsigned char *>(OriginalNsDef->href));
    OriginalNsDef->href = NewDefinedDefaultHref;
  }
  xmlFree(const_cast<unsigned char *>(OriginalDefinedDefaultHref));
  return Error::success();
}

static bool isRecognizedNamespace(const unsigned char *NsHref) {
  for (auto &Ns : MtNsHrefsPrefixes) {
    if (xmlStringsEqual(NsHref, TO_XML_CHAR(Ns.first.c_str())))
      return true;
  }
  return false;
}

static bool hasRecognizedNamespace(XMLNodeImpl Node) {
  return isRecognizedNamespace(Node->ns->href);
}

// Ensure a node's inherited namespace is actually defined in the tree it
// resides in.
static Error reconcileNamespaces(XMLNodeImpl Node) {
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
  return Error::success();
}

// Recursively merge the two given manifest trees, depending on which elements
// are of a mergeable type, and choose namespaces according to which have
// higher priority.
static Error treeMerge(XMLNodeImpl OriginalRoot, XMLNodeImpl AdditionalRoot) {
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
  return Error::success();
}

static void stripComments(XMLNodeImpl Root) {
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
}

// The libxml2 assumes that attributes do not inherit default namespaces,
// whereas the original  mt.exe does make this assumption.  This function
// reconciles this by setting all attributes to have the inherited default
// namespace.
static void setAttributeNamespaces(XMLNodeImpl Node) {
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

// The merging process may create too many prefix defined namespaces.  This
// function removes all unnecessary ones from the tree.
static void checkAndStripPrefixes(XMLNodeImpl Node,
                           std::vector<XMLNsImpl> &RequiredPrefixes) {
  for (XMLNodeImpl Child = Node->children; Child; Child = Child->next) {
    checkAndStripPrefixes(Child, RequiredPrefixes);
  }
  if (Node->ns && Node->ns->prefix != nullptr) {
    XMLNsImpl ClosestDefault = getClosestDefault(Node);
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
      XMLNsImpl ClosestDefault = getClosestDefault(Node);
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
      xmlFreeNs(Def);
      Def = &Temp;
    } else {
      Prev = Def;
    }
  }
}

WindowsManifestMerger::~WindowsManifestMerger() {
  for (auto &Doc : MergedDocs)
    xmlFreeDoc(Doc);
}

Error WindowsManifestMerger::merge(const MemoryBuffer &Manifest) {
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
  return Error::success();
}

std::unique_ptr<MemoryBuffer> WindowsManifestMerger::getMergedManifest() {
  unsigned char *XmlBuff;
  int BufferSize = 0;
  if (CombinedDoc) {
    XMLNodeImpl CombinedRoot = xmlDocGetRootElement(CombinedDoc);
    std::vector<XMLNsImpl> RequiredPrefixes;
    checkAndStripPrefixes(CombinedRoot, RequiredPrefixes);
    std::unique_ptr<xmlDoc> OutputDoc(xmlNewDoc((const unsigned char *)"1.0"));
    xmlDocSetRootElement(OutputDoc.get(), CombinedRoot);
    xmlKeepBlanksDefault(0);
    xmlDocDumpFormatMemoryEnc(OutputDoc.get(), &XmlBuff, &BufferSize, "UTF-8",
                              1);
  }
  if (BufferSize == 0)
    return nullptr;
  return MemoryBuffer::getMemBuffer(
      StringRef(FROM_XML_CHAR(XmlBuff), (size_t)BufferSize));
  return nullptr;
}

#else

WindowsManifestMerger::~WindowsManifestMerger() {}

Error WindowsManifestMerger::merge(const MemoryBuffer &Manifest) {
  return Error::success();
}

std::unique_ptr<MemoryBuffer> WindowsManifestMerger::getMergedManifest() {
  return nullptr;
}

#endif 

void WindowsManifestMerger::errorCallback(void *Ctx, const char *Format, ...) {
  auto *Merger = (WindowsManifestMerger *)Ctx;
  Merger->ParseErrorOccurred = true;
}

Error WindowsManifestMerger::getParseError() {
  if (!ParseErrorOccurred)
    return Error::success();
  return make_error<WindowsManifestError>("invalid xml document");
}
