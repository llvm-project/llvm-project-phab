//===--- Protocol.h - Language Server Protocol Implementation ---*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains structs based on the LSP specification at
// https://github.com/Microsoft/language-server-protocol/blob/master/protocol.md
//
// This is not meant to be a complete implementation, new interfaces are added
// when they're needed.
//
// Each struct has a parse and unparse function, that converts back and forth
// between the struct and a JSON representation.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANGD_PROTOCOL_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANGD_PROTOCOL_H

#include "llvm/ADT/Optional.h"
#include "llvm/Support/YAMLParser.h"
#include <string>
#include <vector>

namespace clang {
namespace clangd {

struct URI {
  std::string uri;
  std::string File;

  static URI fromUri(llvm::StringRef uri);
  static URI fromFile(llvm::StringRef Filename);

  static URI parse(llvm::yaml::ScalarNode *Param);
  static std::string unparse(const URI &U);
};

struct TextDocumentIdentifier {
  /// The text document's URI.
  URI uri;

  static llvm::Optional<TextDocumentIdentifier>
  parse(llvm::yaml::MappingNode *Params);
};

struct Position {
  /// Line position in a document (zero-based).
  unsigned Line;

  /// Character offset on a line in a document (zero-based).
  unsigned LineOffset;

  friend bool operator==(const Position &LHS, const Position &RHS) {
    return std::tie(LHS.Line, LHS.LineOffset) ==
           std::tie(RHS.Line, RHS.LineOffset);
  }

  friend bool operator<(const Position &LHS, const Position &RHS) {
    return std::tie(LHS.Line, LHS.LineOffset) <
           std::tie(RHS.Line, RHS.LineOffset);
  }

  static llvm::Optional<Position> parse(llvm::yaml::MappingNode *Params);
  static std::string unparse(const Position &P);
};

struct Range {
  /// The range's start position.
  Position Start;

  /// The range's end position.
  Position End;

  friend bool operator==(const Range &LHS, const Range &RHS) {
    return std::tie(LHS.Start, LHS.End) == std::tie(RHS.Start, RHS.End);
  }
  friend bool operator<(const Range &LHS, const Range &RHS) {
    return std::tie(LHS.Start, LHS.End) < std::tie(RHS.Start, RHS.End);
  }

  static llvm::Optional<Range> parse(llvm::yaml::MappingNode *Params);
  static std::string unparse(const Range &P);
};

struct TextEdit {
  /// The range of the text document to be manipulated. To insert
  /// text into a document create a range where start === end.
  Range Range;

  /// The string to be inserted. For delete operations use an
  /// empty string.
  std::string NewText;

  static llvm::Optional<TextEdit> parse(llvm::yaml::MappingNode *Params);
  static std::string unparse(const TextEdit &P);
};

struct TextDocumentItem {
  /// The text document's URI.
  URI uri;

  /// The text document's language identifier.
  std::string LanguageId;

  /// The version number of this document (it will strictly increase after each
  int Version;

  /// The content of the opened text document.
  std::string Text;

  static llvm::Optional<TextDocumentItem>
  parse(llvm::yaml::MappingNode *Params);
};

struct DidOpenTextDocumentParams {
  /// The document that was opened.
  TextDocumentItem TextDocument;

  static llvm::Optional<DidOpenTextDocumentParams>
  parse(llvm::yaml::MappingNode *Params);
};

struct DidCloseTextDocumentParams {
  /// The document that was closed.
  TextDocumentIdentifier TextDocument;

  static llvm::Optional<DidCloseTextDocumentParams>
  parse(llvm::yaml::MappingNode *Params);
};

struct TextDocumentContentChangeEvent {
  /// The new text of the document.
  std::string Text;

  static llvm::Optional<TextDocumentContentChangeEvent>
  parse(llvm::yaml::MappingNode *Params);
};

struct DidChangeTextDocumentParams {
  /// The document that did change. The version number points
  /// to the version after all provided content changes have
  /// been applied.
  TextDocumentIdentifier TextDocument;

  /// The actual content changes.
  std::vector<TextDocumentContentChangeEvent> contentChanges;

  static llvm::Optional<DidChangeTextDocumentParams>
  parse(llvm::yaml::MappingNode *Params);
};

struct FormattingOptions {
  /// Size of a tab in spaces.
  unsigned TabSize;

  /// Prefer spaces over tabs.
  bool InsertSpaces;

  static llvm::Optional<FormattingOptions>
  parse(llvm::yaml::MappingNode *Params);
  static std::string unparse(const FormattingOptions &P);
};

struct DocumentRangeFormattingParams {
  /// The document to format.
  TextDocumentIdentifier TextDocument;

  /// The range to format
  Range FormatRange;

  /// The format options
  FormattingOptions Options;

  static llvm::Optional<DocumentRangeFormattingParams>
  parse(llvm::yaml::MappingNode *Params);
};

struct DocumentOnTypeFormattingParams {
  /// The document to format.
  TextDocumentIdentifier TextDocument;

  /// The position at which this request was sent.
  Position RequestPosition;

  /// The character that has been typed.
  std::string TypedCharacter;

  /// The format options.
  FormattingOptions Options;

  static llvm::Optional<DocumentOnTypeFormattingParams>
  parse(llvm::yaml::MappingNode *Params);
};

struct DocumentFormattingParams {
  /// The document to format.
  TextDocumentIdentifier TextDocument;

  /// The format options
  FormattingOptions Options;

  static llvm::Optional<DocumentFormattingParams>
  parse(llvm::yaml::MappingNode *Params);
};

struct Diagnostic {
  /// The range at which the message applies.
  Range Range;

  /// The diagnostic's severity. Can be omitted. If omitted it is up to the
  /// client to interpret diagnostics as error, warning, info or hint.
  int Severity;

  /// The diagnostic's message.
  std::string Message;

  friend bool operator==(const Diagnostic &LHS, const Diagnostic &RHS) {
    return std::tie(LHS.Range, LHS.Severity, LHS.Message) ==
           std::tie(RHS.Range, RHS.Severity, RHS.Message);
  }
  friend bool operator<(const Diagnostic &LHS, const Diagnostic &RHS) {
    return std::tie(LHS.Range, LHS.Severity, LHS.Message) <
           std::tie(RHS.Range, RHS.Severity, RHS.Message);
  }

  static llvm::Optional<Diagnostic> parse(llvm::yaml::MappingNode *Params);
};

struct CodeActionContext {
  /// An array of diagnostics.
  std::vector<Diagnostic> Diagnostics;

  static llvm::Optional<CodeActionContext>
  parse(llvm::yaml::MappingNode *Params);
};

struct CodeActionParams {
  /// The document in which the command was invoked.
  TextDocumentIdentifier TextDocument;

  /// The range for which the command was invoked.
  Range Range;

  /// Context carrying additional information.
  CodeActionContext Context;

  static llvm::Optional<CodeActionParams>
  parse(llvm::yaml::MappingNode *Params);
};

struct TextDocumentPositionParams {
  /// The text document.
  TextDocumentIdentifier TextDocument;

  /// The position inside the text document.
  Position Position;

  static llvm::Optional<TextDocumentPositionParams>
  parse(llvm::yaml::MappingNode *Params);
};

/// The kind of a completion entry.
enum class CompletionItemKind {
  Missing = 0,
  Text = 1,
  Method = 2,
  Function = 3,
  Constructor = 4,
  Field = 5,
  Variable = 6,
  Class = 7,
  Interface = 8,
  Module = 9,
  Property = 10,
  Unit = 11,
  Value = 12,
  Enum = 13,
  Keyword = 14,
  Snippet = 15,
  Color = 16,
  File = 17,
  Reference = 18,
};

/// Defines whether the insert text in a completion item should be interpreted
/// as plain text or a snippet.
enum class InsertTextFormat {
  Missing = 0,
  /// The primary text to be inserted is treated as a plain string.
  PlainText = 1,
  /// The primary text to be inserted is treated as a snippet.
  ///
  /// A snippet can define tab stops and placeholders with `$1`, `$2`
  /// and `${3:foo}`. `$0` defines the final tab stop, it defaults to the end
  /// of the snippet. Placeholders with equal identifiers are linked, that is
  /// typing in one will update others too.
  ///
  /// See also:
  /// https//github.com/Microsoft/vscode/blob/master/src/vs/editor/contrib/snippet/common/snippet.md
  Snippet = 2,
};

struct CompletionItem {
  /// The label of this completion item. By default also the text that is
  /// inserted when selecting this completion.
  std::string Label;

  /// The kind of this completion item. Based of the kind an icon is chosen by
  /// the editor.
  CompletionItemKind Kind = CompletionItemKind::Missing;

  /// A human-readable string with additional information about this item, like
  /// type or symbol information.
  std::string Detail;

  /// A human-readable string that represents a doc-comment.
  std::string Documentation;

  /// A string that should be used when comparing this item with other items.
  /// When `falsy` the label is used.
  std::string SortText;

  /// A string that should be used when filtering a set of completion items.
  /// When `falsy` the label is used.
  std::string FilterText;

  /// A string that should be inserted to a document when selecting this
  /// completion. When `falsy` the label is used.
  std::string InsertText;

  /// The format of the insert text. The format applies to both the `insertText`
  /// property and the `newText` property of a provided `textEdit`.
  InsertTextFormat InsertTextFormat = InsertTextFormat::Missing;

  /// An edit which is applied to a document when selecting this completion.
  /// When an edit is provided `insertText` is ignored.
  ///
  /// Note: The range of the edit must be a single line range and it must
  /// contain the position at which completion has been requested.
  llvm::Optional<TextEdit> Edit;

  /// An optional array of additional text edits that are applied when selecting
  /// this completion. Edits must not overlap with the main edit nor with
  /// themselves.
  std::vector<TextEdit> AdditionalTextEdits;

  // TODO(krasimir): The following optional fields defined by the language
  // server protocol are unsupported:
  //
  // command?: Command - An optional command that is executed *after* inserting
  //                     this completion.
  //
  // data?: any - A data entry field that is preserved on a completion item
  //              between a completion and a completion resolve request.
  static std::string unparse(const CompletionItem &P);
};

} // namespace clangd
} // namespace clang

#endif
