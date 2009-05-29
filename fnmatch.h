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

class FnmatchCompiler;
class FnmatchRule;

typedef std::vector<FnmatchRule*> FnmatchRules;

class FnmatchFunction {
  public:
    FnmatchFunction(FnmatchCompiler* compiler, 
        FnmatchRules::const_iterator& rule_iter,
        const FnmatchRules::const_iterator& rule_end, const std::string& name);
    Function* func;

    FnmatchCompiler* compiler;
    std::vector<FnmatchRule*>::const_iterator rule_iter;
    const std::vector<FnmatchRule*>::const_iterator rule_end;

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
};

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

    void dump(void) const {
      module->dump();
    }

    void Compile(const std::string& pattern, const std::string& name="match");

    void optimize();

    bool run(const char* path);

    FnmatchFunction* function;


    static Module* module;
  private:
    static ExecutionEngine* executionEngine;
    static ExistingModuleProvider* mp;
    static FunctionPassManager* fpm;
};

// represents part of an fnmatch rule
class FnmatchRule {
  public:
    virtual void Compile(FnmatchFunction* compiler, BasicBlock* pre, BasicBlock* post) = 0;
};

// represents the match of a single character
class FnmatchCharacter : public FnmatchRule {
  public:
    FnmatchCharacter(char ch) : character(ch) { }
    virtual void Compile(FnmatchFunction* compiler, BasicBlock* pre, BasicBlock* post);
  private:
    char character;
};

// represents a single character wildcard
class FnmatchSingle : public FnmatchRule {
  public:
    FnmatchSingle() { }
    virtual void Compile(FnmatchFunction* compiler, BasicBlock* pre, BasicBlock* post);
};

// represents a bracket expression
class FnmatchBracket : public FnmatchRule {
  public:
    FnmatchBracket(bool _inverse, const std::string& _characters)
      : inverse(_inverse), characters(_characters) { }
    virtual void Compile(FnmatchFunction* compiler, BasicBlock* pre, BasicBlock* post);
  private:
    bool inverse;
    const std::string characters;
};

// represents a multi-char wildcard
class FnmatchMultiple : public FnmatchRule {
  public:
    FnmatchMultiple() { }
    virtual void Compile(FnmatchFunction* compiler, BasicBlock* pre, BasicBlock* post);
};



// parser to turn strings into fnmatch rules
class FnmatchParser {
  public:
    std::vector<FnmatchRule*> Parse(const std::string& rule);
  private:
    FnmatchRule* ParseOne(std::string::const_iterator& i, const std::string::const_iterator& end);
};
