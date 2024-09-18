//===---- Query.cpp - clang-query query -----------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// Modified by Haggai Nuchi 2024

#include "Query.h"
#include "QuerySession.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Frontend/ASTUnit.h"
#include "llvm/Support/raw_ostream.h"
#include <optional>
#include <sstream>

using namespace clang::ast_matchers;
using namespace clang::ast_matchers::dynamic;

namespace clang {
namespace query {

Query::~Query() {}

bool InvalidQuery::run(llvm::raw_ostream &OS, QuerySession &QS) const {
  OS << ErrStr << "\n";
  return false;
}

bool NoOpQuery::run(llvm::raw_ostream &OS, QuerySession &QS) const {
  return true;
}

bool HelpQuery::run(llvm::raw_ostream &OS, QuerySession &QS) const {
  OS << "Available commands:\n\n"
        "  match MATCHER, m MATCHER          "
        "Match the loaded ASTs against the given matcher.\n"
        "  let NAME MATCHER, l NAME MATCHER  "
        "Give a matcher expression a name, to be used later\n"
        "                                    "
        "as part of other expressions.\n"
        "  set bind-root (true|false)        "
        "Set whether to bind the root matcher to \"root\".\n"
        "  set print-matcher (true|false)    "
        "Set whether to print the current matcher,\n"
        "  set traversal <kind>              "
        "Set traversal kind of clang-query session. Available kinds are:\n"
        "    AsIs                            "
        "Print and match the AST as clang sees it.  This mode is the "
        "default.\n"
        "    IgnoreUnlessSpelledInSource     "
        "Omit AST nodes unless spelled in the source.\n"
        "  set output <feature>              "
        "Set whether to output only <feature> content.\n"
        "  enable output <feature>           "
        "Enable <feature> content non-exclusively.\n"
        "  disable output <feature>          "
        "Disable <feature> content non-exclusively.\n"
        "  quit, q                           "
        "Terminates the query session.\n\n"
        "Several commands accept a <feature> parameter. The available features "
        "are:\n\n"
        "  print                             "
        "Pretty-print bound nodes.\n"
        "  diag                              "
        "Diagnostic location for bound nodes.\n"
        "  detailed-ast                      "
        "Detailed AST output for bound nodes.\n"
        "  srcloc                            "
        "Source locations and ranges for bound nodes.\n"
        "  dump                              "
        "Detailed AST output for bound nodes (alias of detailed-ast).\n\n";
  return true;
}

bool QuitQuery::run(llvm::raw_ostream &OS, QuerySession &QS) const {
  QS.Terminate = true;
  return true;
}

namespace {

struct CollectBoundNodes : MatchFinder::MatchCallback {
  std::vector<BoundNodes> &Bindings;
  CollectBoundNodes(std::vector<BoundNodes> &Bindings) : Bindings(Bindings) {}
  void run(const MatchFinder::MatchResult &Result) override {
    Bindings.push_back(Result.Nodes);
  }
};

} // namespace

bool MatchQuery::run(llvm::raw_ostream &OS, QuerySession &QS) const {
  auto DiagnosticIt = QS.NamedValues.find("Diagnostic");
  if (DiagnosticIt == QS.NamedValues.end() || !DiagnosticIt->second.isString()) {
    return true;
  }
  const std::string& DiagnosticMsg = DiagnosticIt->second.getString();

  std::vector<std::string> args;
  auto DiagnosticArgsIt = QS.NamedValues.find("DiagnosticArgs");
  if (DiagnosticArgsIt != QS.NamedValues.end() && DiagnosticArgsIt->second.isString()) {
    std::istringstream iss(DiagnosticArgsIt->second.getString());
    std::string word;
    while (iss >> word) {
      args.push_back(word);
    }
  }

  if (args.size() == 0) {
    args.push_back("root");
  }

  unsigned MatchCount = 0;

  MatchFinder Finder;
  std::vector<BoundNodes> Matches;
  DynTypedMatcher MaybeBoundMatcher = Matcher;
  if (QS.BindRoot) {
    std::optional<DynTypedMatcher> M = Matcher.tryBind("root");
    if (M)
      MaybeBoundMatcher = *M;
  }
  CollectBoundNodes Collect(Matches);
  if (!Finder.addDynamicMatcher(MaybeBoundMatcher, &Collect)) {
    OS << "Not a valid top-level matcher.\n";
    return false;
  }

  auto &Ctx = QS.Ctx;
  Ctx.getParentMapContext().setTraversalKind(QS.TK);
  Finder.matchAST(Ctx);

  unsigned customID = Ctx.getDiagnostics().getDiagnosticIDs()->getCustomDiagID(
    DiagnosticIDs::Warning, DiagnosticMsg
  );

  for (auto MI = Matches.begin(), ME = Matches.end(); MI != ME; ++MI) {
    std::vector<DynTypedNode> nodeArgs;
    nodeArgs.reserve(args.size());

    for (const auto& arg : args) {
      auto it = MI->getMap().find(arg);
      if (it == MI->getMap().end()) {
        OS << "Couldn't find bound node '" << arg << "'\n";
        return false;
      }
      nodeArgs.push_back(it->second);
    }

    clang::SourceRange Range = nodeArgs[0].getSourceRange();
    if (Range.isValid()) {
      auto Diag = Ctx.getDiagnostics().Report(FullSourceLoc(Range.getBegin(), Ctx.getSourceManager()), customID);
      Diag << Range;
      for (const auto& arg : nodeArgs) {
        std::string s;
        llvm::raw_string_ostream os(s);
        arg.print(os, Ctx.getPrintingPolicy());
        Diag << s;
      }
    }
  }

  return true;
}

bool LetQuery::run(llvm::raw_ostream &OS, QuerySession &QS) const {
  if (Value) {
    QS.NamedValues[Name] = Value;
  } else {
    QS.NamedValues.erase(Name);
  }
  return true;
}

#ifndef _MSC_VER
const QueryKind SetQueryKind<bool>::value;
const QueryKind SetQueryKind<OutputKind>::value;
#endif

} // namespace query
} // namespace clang
