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

#include <fnmatch.h>

using namespace llvm;

static ExecutionEngine* executionEngine;
static Module* module;

class FnmatchCompiler {
  public:
    Function* func;

    FnmatchCompiler() {
      previous = NULL;
    }

    void dump(void) const {
      module->dump();
    }

    void compile(const char* pattern, const char* name="match");

    void optimize();

    bool run(const char* path);


  private:
    Value* path_ptr; // Value for the path argument to the function
    Value* index_ptr; // Value representing the local variable for the offset within the path
    BasicBlock* previous; // the most recently created block

    // helpers
    Value* loadPathCharacter(BasicBlock* basicBlock);
    void consumePathCharacter(BasicBlock* basicBlock);

    // complex expressions
    void bracketExpression(const char*& pattern);
    void wildcardExpression(const char*& pattern);
};

void
FnmatchCompiler::compile(const char* pattern, const char* name) {
  // create a function of type i1(char*)
  std::vector<const Type*> args_type;
  args_type.push_back(PointerType::get(IntegerType::get(8), 0));
  FunctionType *func_type = FunctionType::get(IntegerType::get(1), args_type, false);
  func = Function::Create(func_type, Function::ExternalLinkage, std::string(name), module);

  // get the path argument 
  Function::arg_iterator args = func->arg_begin();
  path_ptr = args++;
  path_ptr->setName("path");

  // create an entry point for the function
  BasicBlock* entry = BasicBlock::Create("entry", func);
  // create a variable to track the position within the path
  index_ptr = new AllocaInst(IntegerType::get(32), "index_ptr", entry);
  // start with zero in it
  new StoreInst(ConstantInt::get(IntegerType::get(32), 0), index_ptr, entry);
  // we want to link the entry block into the next one we create...
  previous = entry;

  // true and false
  ConstantInt* true1 = ConstantInt::get(IntegerType::get(1), 1);
  ConstantInt* false1 = ConstantInt::get(IntegerType::get(1), 0);

  // create a block that returns false
  BasicBlock* return_false = BasicBlock::Create("return_false", func);
  ReturnInst::Create(false1, return_false);

  // the basic blocks we use:
  BasicBlock* test = NULL; // where we load a character from the path and test it
  BasicBlock* cont = NULL; // where we continue on to the next character
 
  while(*pattern) {
    //printf("compiling: '%c'\n", *pattern);
    // make a basicblock for this test
    test = BasicBlock::Create("test", func);
    // if needed, make the last cont block branch to this test block...
    if (previous) {
      BranchInst::Create(test, previous);
      previous = NULL;
    }
    // make a new cont block
    cont = BasicBlock::Create("cont", func);
    previous = cont;

    if (*pattern == '?') {
      // match any character, so just check against \0
      Value* path_char = loadPathCharacter(test);
      ICmpInst* cmp_chr = new ICmpInst(ICmpInst::ICMP_EQ, path_char, ConstantInt::get(IntegerType::get(8), 0), "", test);
      BranchInst::Create(return_false, cont, cmp_chr, test);
      pattern++;
      consumePathCharacter(cont);
      continue;
    } else if (*pattern == '[') {
      bracketExpression(pattern);
      continue;
    } else if (*pattern == '*') {
      wildcardExpression(pattern);
      continue;
    } else if (*pattern == '\\') {
      // escape character, treat the next character literally
      pattern++;
    }
    // check the loaded value againt the supplied string
    Value* path_char = loadPathCharacter(test);
    ICmpInst* cmp_chr = new ICmpInst(ICmpInst::ICMP_NE, path_char, ConstantInt::get(IntegerType::get(8), *pattern), "", test);
    BranchInst::Create(return_false, cont, cmp_chr, test);

    // move to the next character in the pattern
    pattern++;
    // move to the next character in the path
    consumePathCharacter(cont);

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
}

Value* 
FnmatchCompiler::loadPathCharacter(BasicBlock* basicBlock) {
  // load the index
  Value* index = new LoadInst(index_ptr, "index", basicBlock);
  // get a pointer to the path character
  GetElementPtrInst* path_char_ptr = GetElementPtrInst::Create(path_ptr, index, "", basicBlock);
  // load the value
  LoadInst* path_char = new LoadInst(path_char_ptr, "path_char", basicBlock);
  return path_char;
}

void
FnmatchCompiler::consumePathCharacter(BasicBlock* basicBlock) {
  // load the index
  Value* index = new LoadInst(index_ptr, "index", basicBlock);
  // increment
  index = BinaryOperator::CreateAdd(index, ConstantInt::get(IntegerType::get(32), 1), "increment", basicBlock);
  // store the index
  new StoreInst(index, index_ptr, basicBlock);
}

void
FnmatchCompiler::bracketExpression(const char*& pattern) {
  // start of a bracket expression
  pattern++;
  bool matching = true;
  if (*pattern == '!') {
    matching = false;
    pattern++;
  }
  // FIXME: if 1st char is ] then that doesn't terminate the bracket expression
  // FIXME: support ranges...
  while(*pattern && *pattern != ']') {
    if (matching) {
      //ICmpInst* cmp_chr = new ICmpInst(ICmpInst::ICMP_NE, path_char, ConstantInt::get(IntegerType::get(8), *pattern), "", test);
    } else {
    }
  }
}

void
FnmatchCompiler::wildcardExpression(const char*& pattern) {
}

void
FnmatchCompiler::optimize() {
  /// optimization
  ExistingModuleProvider mp(module);
  FunctionPassManager fpm(&mp);
    
  // Set up the optimizer pipeline.  Start with registering info about how the
  // target lays out data structures.
  fpm.add(new TargetData(*executionEngine->getTargetData()));
  // Do simple "peephole" optimizations and bit-twiddling optzns.
  fpm.add(createInstructionCombiningPass());
  // Reassociate expressions.
  fpm.add(createReassociatePass());
  // Eliminate Common SubExpressions.
  fpm.add(createGVNPass());
  // Simplify the control flow graph (deleting unreachable blocks, etc).
  fpm.add(createCFGSimplificationPass());
  fpm.run(*func);
}

bool
FnmatchCompiler::run(const char* path) {
  // okay, let's run this thing
  void* func_void_ptr = executionEngine->getPointerToFunction(func);
  bool (*func_ptr)(const char*) = (bool (*)(const char*))func_void_ptr;
  return (func_ptr)(path);
}

void
test(const char* pattern, const char* path) {
  FnmatchCompiler* compiler = new FnmatchCompiler();
  compiler->compile(pattern);
/*
  compiler->dump();
  printf("----------------------------\n");
  printf("optimizing\n");
  compiler->optimize();
  printf("optimized\n");
  compiler->dump();
  printf("dumped\n");
*/
  bool result = compiler->run(path);
  delete compiler;
  bool expected = (fnmatch(pattern, path, 0) == 0);
  printf("[%c] fnmatch(\"%s\", \"%s\") = %s, expected %s\n", 
      (expected == result)?'+':'-', pattern, path, result?"true":"false", 
      expected?"true":"false");
}

int main(int argc, char**argv) {
  // important initialization - do this first
  module = new Module("fnmatch");
  executionEngine = ExecutionEngine::create(module);

  test("a", "a");
  test(".", "a");
  test("foo", "foo");
  test("f?o", "foo");
}
