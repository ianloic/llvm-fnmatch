/* fnmatch on llvm. a trivial language */
/* 8-bit chars only, because I'm lazy 
 * and I don't feel like learning c++ unicde today */

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

using namespace llvm;

static Module *TheModule;
static IRBuilder<> Builder;
static ExecutionEngine* TheExecutionEngine;

int main(int argc, char**argv) {
  if (argc != 3) {
    printf("%s <pattern> <string>\n", argv[0]);
    return -1;
  }

  // create a new module
  TheModule = new Module("fnmatch module");

  // create a function of type i1(char*)
  std::vector<const Type*> args_type;
  args_type.push_back(PointerType::get(IntegerType::get(8), 0));
  FunctionType *func_type = FunctionType::get(IntegerType::get(1), args_type, false);
  Function *func = Function::Create(func_type, Function::ExternalLinkage, std::string("match"), TheModule);

  // get the path argument 
  Function::arg_iterator args = func->arg_begin();
  Value* path_ptr = args++;
  path_ptr->setName("path");

  // true and false
  ConstantInt* true1 = ConstantInt::get(IntegerType::get(1), 1);
  ConstantInt* false1 = ConstantInt::get(IntegerType::get(1), 0);

  // create a block that returns false
  BasicBlock* return_false = BasicBlock::Create("return_false", func);
  ReturnInst::Create(false1, return_false);

  // the basic blocks we use:
  BasicBlock* test = NULL; // where we load a character from the path and test it
  BasicBlock* cont = NULL; // where we continue on to the next character
  
  size_t pattern_length = strlen(argv[1]);
  for (size_t i = 0; i < pattern_length; i++) {
    // make a basicblock for this test
    test = BasicBlock::Create("test", func);
    // if needed, make the last cont block branch to this test block...
    if (cont) {
      BranchInst::Create(test, cont);
    }
    // make a new cont block
    cont = BasicBlock::Create("cont", func);
    // get a pointer to the path character
    GetElementPtrInst* path_char_ptr = GetElementPtrInst::Create(path_ptr, 
        ConstantInt::get(IntegerType::get(32), i), "", test);
    // load the value
    LoadInst* path_char = new LoadInst(path_char_ptr, "path_char", test);
    // check the loaded value againt the supplied string
    ICmpInst* is_nul = new ICmpInst(ICmpInst::ICMP_NE, path_char, ConstantInt::get(IntegerType::get(8), argv[1][i]), "", test);
    BranchInst::Create(return_false, cont, is_nul, test);

  }

  // okay, everything matched, return true
  BasicBlock* return_true = BasicBlock::Create("return_true", func);
  ReturnInst::Create(true1, return_true);
  if (cont) {
    BranchInst::Create(return_true, cont);
  }

  // move return_false after return_true
  return_false->moveAfter(return_true);

  // check the function, eh?
  verifyFunction(*func);

  //TheModule->dump();

  // Create the JIT.
  TheExecutionEngine = ExecutionEngine::create(TheModule);


  /// optimization
  ExistingModuleProvider OurModuleProvider(TheModule);
  FunctionPassManager OurFPM(&OurModuleProvider);
    
  // Set up the optimizer pipeline.  Start with registering info about how the
  // target lays out data structures.
  OurFPM.add(new TargetData(*TheExecutionEngine->getTargetData()));
  // Do simple "peephole" optimizations and bit-twiddling optzns.
  OurFPM.add(createInstructionCombiningPass());
  // Reassociate expressions.
  OurFPM.add(createReassociatePass());
  // Eliminate Common SubExpressions.
  OurFPM.add(createGVNPass());
  // Simplify the control flow graph (deleting unreachable blocks, etc).
  OurFPM.add(createCFGSimplificationPass());
  OurFPM.run(*func);

  printf("----------------------------\n");
  TheModule->dump();


  // okay, let's run this thing
  void* func_void_ptr = TheExecutionEngine->getPointerToFunction(func);
  bool (*func_ptr)(const char*) = (bool (*)(const char*))func_void_ptr;
  bool result = (func_ptr)(argv[2]);
  printf(" result = %s\n", result?"true":"false");
}
