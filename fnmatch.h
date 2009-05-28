// a parser for fnmatch rules

#include "llvm/DerivedTypes.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/Module.h"
#include "llvm/ModuleProvider.h"
#include "llvm/PassManager.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Support/IRBuilder.h"
#include "llvm/Function.h"

#include <string>
#include <vector>

using namespace llvm;

class FnmatchCompiler {
  public:
    static void Initialize() {
      module = new Module("fnmatch");
      executionEngine = ExecutionEngine::create(module);
      mp = new ExistingModuleProvider(module);
      fpm = new FunctionPassManager(mp);
      // Set up the optimizer pipeline.  Start with registering info about how the
      // target lays out data structures.
      fpm->add(new TargetData(*executionEngine->getTargetData()));
      // Do simple "peephole" optimizations and bit-twiddling optzns.
      fpm->add(createInstructionCombiningPass());
      // Reassociate expressions.
      fpm->add(createReassociatePass());
      // Eliminate Common SubExpressions.
      fpm->add(createGVNPass());
      // Simplify the control flow graph (deleting unreachable blocks, etc).
      fpm->add(createCFGSimplificationPass());
    }

    Function* func;

    FnmatchCompiler() {
      previous = NULL;
    }

    void dump(void) const {
      module->dump();
    }

    void Compile(const std::string& pattern, const std::string& name="match");

    void optimize();

    bool run(const char* path);


    Value* path_ptr; // Value for the path argument to the function
    Value* index_ptr; // Value representing the local variable for the offset within the path
    BasicBlock* previous; // the most recently created block

    // blocks that we might want to branch to
    BasicBlock* return_false;
    BasicBlock* return_true;

    // helpers
    Value* loadPathCharacter(BasicBlock* basicBlock);
    void consumePathCharacter(BasicBlock* basicBlock);
    BasicBlock* firstBlock(const std::string& name);
    void lastBlock(BasicBlock* block);

  private:
    static ExecutionEngine* executionEngine;
    static Module* module;
    static ExistingModuleProvider* mp;
    static FunctionPassManager* fpm;

    // complex expressions
    void bracketExpression(const char*& pattern);
    void wildcardExpression(const char*& pattern);
};

// represents part of an fnmatch rule
class FnmatchRule {
  public:
    virtual void Compile(FnmatchCompiler* compiler) = 0;
};

// represents the match of a single character
class FnmatchCharacter : public FnmatchRule {
  public:
    FnmatchCharacter(char ch) : character(ch) { }
    virtual void Compile(FnmatchCompiler* compiler);
  private:
    char character;
};

// represents a single character wildcard
class FnmatchSingle : public FnmatchRule {
  public:
    FnmatchSingle() { }
    virtual void Compile(FnmatchCompiler* compiler);
};

// represents a bracket expression
class FnmatchBracket : public FnmatchRule {
  public:
    FnmatchBracket(bool _inverse, const std::string& _characters)
      : inverse(_inverse), characters(_characters) { }
    virtual void Compile(FnmatchCompiler* compiler);
  private:
    bool inverse;
    const std::string characters;
};

// represents a multi-char wildcard
class FnmatchMultiple : public FnmatchRule {
  public:
    FnmatchMultiple() { }
    virtual void Compile(FnmatchCompiler* compiler);
};



// parser to turn strings into fnmatch rules
class FnmatchParser {
  public:
    std::vector<FnmatchRule*> Parse(const std::string& rule);
  private:
    FnmatchRule* ParseOne(std::string::const_iterator& i, const std::string::const_iterator& end);
};
