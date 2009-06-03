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
    Value* pathCharacterPtr(BasicBlock* basicBlock);
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

      // set up the whole module pipeline
      pm = new PassManager();
      pm->add(new TargetData(module));

      // Set up the function optimizer pipeline.  
      mp = new ExistingModuleProvider(module);
      fpm = new FunctionPassManager(mp);
      // Start with registering info about how the
      // target lays out data structures.
      fpm->add(new TargetData(module));
      // Do simple "peephole" optimizations and bit-twiddling optzns.
      fpm->add(createInstructionCombiningPass());
      // Reassociate expressions.
      fpm->add(createReassociatePass());
      // Eliminate Common SubExpressions.
      fpm->add(createGVNPass());
      // Simplify the control flow graph (deleting unreachable blocks, etc).
      fpm->add(createCFGSimplificationPass());


      pm->add(createVerifierPass());                  // Verify that input is correct

      //pm->add(createLowerSetJmpPass());          // Lower llvm.setjmp/.longjmp

      //pm->add(createRaiseAllocationsPass());     // call %malloc -> malloc inst
      pm->add(createCFGSimplificationPass());    // Clean up disgusting code
      pm->add(createPromoteMemoryToRegisterPass());// Kill useless allocas
      //pm->add(createGlobalOptimizerPass());      // Optimize out global vars
      //pm->add(createGlobalDCEPass());            // Remove unused fns and globs
      //pm->add(createIPConstantPropagationPass());// IP Constant Propagation
      //pm->add(createDeadArgEliminationPass());   // Dead argument elimination
      pm->add(createInstructionCombiningPass()); // Clean up after IPCP & DAE
      pm->add(createCFGSimplificationPass());    // Clean up after IPCP & DAE

      //pm->add(createPruneEHPass());              // Remove dead EH info
      //pm->add(createFunctionAttrsPass());        // Deduce function attrs

      //pm->add(createFunctionInliningPass());   // Inline small functions
      //pm->add(createArgumentPromotionPass());    // Scalarize uninlined fn args

      pm->add(createSimplifyLibCallsPass());     // Library Call Optimizations
      pm->add(createInstructionCombiningPass()); // Cleanup for scalarrepl.
      pm->add(createJumpThreadingPass());        // Thread jumps.
      pm->add(createCFGSimplificationPass());    // Merge & remove BBs
      pm->add(createScalarReplAggregatesPass()); // Break up aggregate allocas
      pm->add(createInstructionCombiningPass()); // Combine silly seq's
      pm->add(createCondPropagationPass());      // Propagate conditionals

      pm->add(createTailCallEliminationPass());  // Eliminate tail calls
      pm->add(createCFGSimplificationPass());    // Merge & remove BBs
      pm->add(createReassociatePass());          // Reassociate expressions
      pm->add(createLoopRotatePass());
      pm->add(createLICMPass());                 // Hoist loop invariants
      pm->add(createLoopUnswitchPass());         // Unswitch loops.
      pm->add(createLoopIndexSplitPass());       // Index split loops.
      // FIXME : Removing instcombine causes nestedloop regression.
      pm->add(createInstructionCombiningPass());
      pm->add(createIndVarSimplifyPass());       // Canonicalize indvars
      pm->add(createLoopDeletionPass());         // Delete dead loops
      pm->add(createLoopUnrollPass());           // Unroll small loops
      pm->add(createInstructionCombiningPass()); // Clean up after the unroller
      pm->add(createGVNPass());                  // Remove redundancies
      pm->add(createMemCpyOptPass());            // Remove memcpy / form memset
      pm->add(createSCCPPass());                 // Constant prop with SCCP

      // Run instcombine after redundancy elimination to exploit opportunities
      // opened up by them.
      pm->add(createInstructionCombiningPass());
      pm->add(createCondPropagationPass());      // Propagate conditionals

      pm->add(createDeadStoreEliminationPass()); // Delete dead stores
      pm->add(createAggressiveDCEPass());        // Delete dead instructions
      pm->add(createCFGSimplificationPass());    // Merge & remove BBs
      //pm->add(createStripDeadPrototypesPass());  // Get rid of dead prototypes
      //pm->add(createDeadTypeEliminationPass());  // Eliminate dead types
      //pm->add(createConstantMergePass());        // Merge dup global constants



    fpm->add(createCFGSimplificationPass());
    fpm->add(createScalarReplAggregatesPass());
    fpm->add(createInstructionCombiningPass());

    //pm->add(createRaiseAllocationsPass());      // call %malloc -> malloc inst
    pm->add(createCFGSimplificationPass());       // Clean up disgusting code
    pm->add(createPromoteMemoryToRegisterPass()); // Kill useless allocas
    //pm->add(createGlobalOptimizerPass());       // OptLevel out global vars
    //pm->add(createGlobalDCEPass());             // Remove unused fns and globs
    //pm->add(createIPConstantPropagationPass()); // IP Constant Propagation
    //pm->add(createDeadArgEliminationPass());    // Dead argument elimination
    pm->add(createInstructionCombiningPass());    // Clean up after IPCP & DAE
    pm->add(createCFGSimplificationPass());       // Clean up after IPCP & DAE
    //pm->add(createPruneEHPass());               // Remove dead EH info
    //pm->add(createFunctionAttrsPass());         // Deduce function attrs
    //pm->add(createFunctionInliningPass());      // Inline small functions
    //pm->add(createArgumentPromotionPass());   // Scalarize uninlined fn args
    pm->add(createSimplifyLibCallsPass());    // Library Call Optimizations
    pm->add(createInstructionCombiningPass());  // Cleanup for scalarrepl.
    pm->add(createJumpThreadingPass());         // Thread jumps.
    pm->add(createCFGSimplificationPass());     // Merge & remove BBs
    pm->add(createScalarReplAggregatesPass());  // Break up aggregate allocas
    pm->add(createInstructionCombiningPass());  // Combine silly seq's
    pm->add(createCondPropagationPass());       // Propagate conditionals
    pm->add(createTailCallEliminationPass());   // Eliminate tail calls
    pm->add(createCFGSimplificationPass());     // Merge & remove BBs
    pm->add(createReassociatePass());           // Reassociate expressions
    pm->add(createLoopRotatePass());            // Rotate Loop
    pm->add(createLICMPass());                  // Hoist loop invariants
    pm->add(createLoopUnswitchPass());
    pm->add(createLoopIndexSplitPass());        // Split loop index
    pm->add(createInstructionCombiningPass());  
    pm->add(createIndVarSimplifyPass());        // Canonicalize indvars
    pm->add(createLoopDeletionPass());          // Delete dead loops
    pm->add(createLoopUnrollPass());          // Unroll small loops
    pm->add(createInstructionCombiningPass());  // Clean up after the unroller
    pm->add(createGVNPass());                   // Remove redundancies
    pm->add(createMemCpyOptPass());             // Remove memcpy / form memset
    pm->add(createSCCPPass());                  // Constant prop with SCCP
  
    // Run instcombine after redundancy elimination to exploit opportunities
    // opened up by them.
    pm->add(createInstructionCombiningPass());
    pm->add(createCondPropagationPass());       // Propagate conditionals
    pm->add(createDeadStoreEliminationPass());  // Delete dead stores
    pm->add(createAggressiveDCEPass());   // Delete dead instructions
    pm->add(createCFGSimplificationPass());     // Merge & remove BBs
  
    //pm->add(createStripDeadPrototypesPass());   // Get rid of dead prototypes
    //pm->add(createDeadTypeEliminationPass());   // Eliminate dead types
  
    //pm->add(createConstantMergePass());       // Merge dup global constants 
  

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
    static PassManager* pm;
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
