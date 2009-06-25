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

class Test {
  public:
    ExecutionEngine* executionEngine;
    Module* module;
    ExistingModuleProvider* mp;
    FunctionPassManager* fpm;

    Test(const char* name) {
      module = new Module(name);
      executionEngine = ExecutionEngine::create(module);
      mp = new ExistingModuleProvider(module);
      fpm = new FunctionPassManager(mp);
    }

    void compile();
};

void
Test::compile() {
  // create a function of type i1(i32)
  std::vector<const Type*> args_type;
  args_type.push_back(IntegerType::get(32));
  FunctionType *func_type = FunctionType::get(IntegerType::get(1), args_type, false);
  Function* func = Function::Create(func_type, Function::ExternalLinkage, std::string("test"), module);

  // that returns false
  BasicBlock* entry = BasicBlock::Create("entry", func);
  ReturnInst::Create(ConstantInt::get(IntegerType::get(1), 0), entry);

  // optimize
  fpm->add(new TargetData(*executionEngine->getTargetData()));
  fpm->add(createCFGSimplificationPass());
  fpm->run(*func);
}

int main(int argc, char**argv) {
  Test* test1 = new Test("test1");
  test1->compile();

  Test* test2 = new Test("test2");
  test2->compile();

  return 0;
}

