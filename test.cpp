#include "llvm/DerivedTypes.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/Module.h"
#include "llvm/ModuleProvider.h"
#include "llvm/PassManager.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Support/IRBuilder.h"

#include <string>

#include <fnmatch.h>

using namespace llvm;

class Thing {
  public:
    ExecutionEngine* executionEngine;
    Module* module;
    ExistingModuleProvider* mp;
    FunctionPassManager* fpm;

    Thing() {
      module = new Module("test");
      executionEngine = ExecutionEngine::create(module);
      mp = new ExistingModuleProvider(module);
      fpm = new FunctionPassManager(mp);
    }

    void compile();

    void dump() {
      printf("module=%p\n", module);
      module->dump();
    }
};

void
Thing::compile() {
  // create a function of type i1(i32)
  std::vector<const Type*> args_type;
  args_type.push_back(IntegerType::get(32));
  FunctionType *func_type = FunctionType::get(IntegerType::get(1), args_type, false);
  Function* func = Function::Create(func_type, Function::ExternalLinkage, std::string("test"), module);

  // get the argument 
  Function::arg_iterator args = func->arg_begin();
  Value* arg = args++;
  arg->setName("arg");

  // create an entry point for the function
  BasicBlock* entry = BasicBlock::Create("entry", func);
  // returns false
  ReturnInst::Create(ConstantInt::get(IntegerType::get(1), 0), entry);

  // optimize
  
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
  fpm->run(*func);
  this->dump();
}

int main(int argc, char**argv) {
  Thing* thing1 = new Thing();
  thing1->compile();
  //thing1->dump();

  return 0;
}

