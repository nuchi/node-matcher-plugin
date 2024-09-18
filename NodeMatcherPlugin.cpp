#include "clang/Frontend/ASTUnit.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/MemoryBuffer.h"
#include "Query.h"
#include "QueryParser.h"
#include "QuerySession.h"

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tooling;
using namespace llvm;

namespace {

cl::list<std::string> QueryFiles(cl::Positional,
                                 cl::desc("Specify the clang-query script files"),
                                 cl::value_desc("filenames"));
std::vector<std::unique_ptr<MemoryBuffer>> FileContents;

class NodeMatcherConsumer : public ASTConsumer {
private:
  CompilerInstance &CI;

public:
  NodeMatcherConsumer(CompilerInstance &CI) : CI(CI) {}

  void HandleTranslationUnit(ASTContext &Context) override {
    for (size_t i = 0; i < FileContents.size(); ++i) {
      StringRef FileContent = FileContents.at(i)->getBuffer();
      clang::query::QuerySession QS(Context);
      while (!FileContent.empty()) {
        clang::query::QueryRef Q = clang::query::QueryParser::parse(FileContent, QS);
        if (!Q->run(llvm::outs(), QS)) {
          llvm::errs() << "Error: Query execution failed in " << QueryFiles[i] << "\n";
          return;
        }
        FileContent = Q->RemainingContent;
      }
    }
  }
};

class NodeMatcherAction : public PluginASTAction {
protected:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 StringRef) override {
    return std::make_unique<NodeMatcherConsumer>(CI);
  }

  bool ParseArgs(const CompilerInstance &CI,
                 const std::vector<std::string> &args) override {
    std::vector<const char*> ccargs;
    ccargs.push_back("QueryPlugin");
    for (const auto& arg : args) {
      ccargs.push_back(arg.c_str());
    }
    cl::ParseCommandLineOptions(ccargs.size(), ccargs.data());

    if (QueryFiles.empty()) {
      llvm::errs() << "Error: No query file specified, pass at least one query file with -fplugin-arg-NodeMatcherPlugin-/path/to/file\n";
      return false;
    }
    FileContents.reserve(QueryFiles.size());
    for (const auto& QueryFile : QueryFiles) {
      auto Buffer = llvm::MemoryBuffer::getFile(QueryFile);
      if (!Buffer) {
        llvm::errs() << "Error: Cannot open " << QueryFile << ": "
                     << Buffer.getError().message() << "\n";
        return false;
      }
      FileContents.push_back(std::move(Buffer.get()));
    }

    return true;
  }

  PluginASTAction::ActionType getActionType() override {
    return AddAfterMainAction;
  }
};

}

static FrontendPluginRegistry::Add<NodeMatcherAction>
X("NodeMatcherPlugin", "Run clang-query scripts and generate warnings for matches");
