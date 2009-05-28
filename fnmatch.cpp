/* fnmatch on llvm. a trivial language */
/* 8-bit chars only, because I'm lazy 
 * and I don't feel like learning c++ unicde today */

#include "fnmatch.h"

#include <string>

#include <fnmatch.h>

using namespace llvm;


Module* FnmatchCompiler::module = NULL;
ExecutionEngine* FnmatchCompiler::executionEngine = NULL;
ExistingModuleProvider* FnmatchCompiler::mp = NULL;
FunctionPassManager* FnmatchCompiler::fpm = NULL;


void
FnmatchCompiler::Compile(const std::string& pattern, const std::string& name) {
  // create a function of type i1(char*)
  std::vector<const Type*> args_type;
  args_type.push_back(PointerType::get(IntegerType::get(8), 0));
  FunctionType *func_type = FunctionType::get(IntegerType::get(1), args_type, false);
  func = Function::Create(func_type, Function::ExternalLinkage, name, module);

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

  // create a block that returns false
  return_false = BasicBlock::Create("return_false", func);
  ReturnInst::Create(ConstantInt::get(IntegerType::get(1), 0), return_false);

  // create a block that returns true
  return_true = BasicBlock::Create("return_true", func);
  ReturnInst::Create(ConstantInt::get(IntegerType::get(1), 1), return_true);

  FnmatchParser parser;
  std::vector<FnmatchRule*> rules = parser.Parse(pattern);

  for (std::vector<FnmatchRule*>::const_iterator i = rules.begin();
      i < rules.end(); i++) {
    FnmatchRule* rule = *i;
    rule->Compile(this);
  }

 
  // okay, everything matched, return true
  BranchInst::Create(return_true, previous);

  // move return_false after return_true
  //return_false->moveAfter(return_true);

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

BasicBlock* 
FnmatchCompiler::firstBlock(const std::string& name) {
  // create a block
  BasicBlock* block = BasicBlock::Create(name, func);

  // if needed, make the last cont block branch to this block...
  if (previous) {
    BranchInst::Create(block, previous);
    previous = NULL;
  }

  return block;
}

void 
FnmatchCompiler::lastBlock(BasicBlock* block) {
  // save off this block...
  previous = block;
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
FnmatchCompiler::optimize() {
  fpm->run(*func);
}

bool
FnmatchCompiler::run(const char* path) {
  // okay, let's run this thing
  void* func_void_ptr = executionEngine->getPointerToFunction(func);
  bool (*func_ptr)(const char*) = (bool (*)(const char*))func_void_ptr;
  return (func_ptr)(path);
}




void
FnmatchCharacter::Compile(FnmatchCompiler* compiler) {
  // create a block for the test
  BasicBlock* test = compiler->firstBlock("test");
  // create a block to continue to on success
  BasicBlock* cont = BasicBlock::Create("cont", compiler->func);

  // load the path character
  Value* path_char = compiler->loadPathCharacter(test);
  // compare the path character with the rule character
  ICmpInst* cmp_chr = new ICmpInst(ICmpInst::ICMP_NE, path_char, 
      ConstantInt::get(IntegerType::get(8), character), "", test);
  // branch
  BranchInst::Create(compiler->return_false, cont, cmp_chr, test);

  // mark the cont block as the final one for this section
  compiler->lastBlock(cont);
}


void
FnmatchSingle::Compile(FnmatchCompiler* compiler) {
  // create a block for the test
  BasicBlock* test = compiler->firstBlock("test");
  // create a block to continue to on success
  BasicBlock* cont = BasicBlock::Create("cont", compiler->func);

  // match any character, so just check against \0
  Value* path_char = compiler->loadPathCharacter(test);
  ICmpInst* cmp_chr = new ICmpInst(ICmpInst::ICMP_EQ, path_char, 
      ConstantInt::get(IntegerType::get(8), 0), "", test);
  BranchInst::Create(compiler->return_false, cont, cmp_chr, test);
  compiler->consumePathCharacter(cont);

  // mark the cont block as the final one for this section
  compiler->lastBlock(cont);
}

void
FnmatchBracket::Compile(FnmatchCompiler* compiler) {
  // create a block for the test
  BasicBlock* test = compiler->firstBlock("test");
  // create a block to continue to on success
  BasicBlock* cont = BasicBlock::Create("cont", compiler->func);

  // load the path character
  Value* path_char = compiler->loadPathCharacter(test);

  BasicBlock* matched;
  BasicBlock* unmatched;

  if (inverse) {
    // any match -> return false
    matched = compiler->return_false;
    // no match -> continue
    unmatched = cont;
  } else {
    // any match -> cont
    matched = cont;
    // no match -> return false
    unmatched = compiler->return_false;
  }

  // switch on the path character
  SwitchInst* sw = SwitchInst::Create(path_char, unmatched, 
      characters.size(), test);
  // for each character in the set...
  for (std::string::const_iterator i = characters.begin();
      i < characters.end(); i++) {
    sw->addCase(ConstantInt::get(IntegerType::get(8), *i), matched);
  }

  // mark the cont block as the final one for this section
  compiler->lastBlock(cont);
}



void
FnmatchMultiple::Compile(FnmatchCompiler* compiler) {
  throw std::string("* not implemented");
}
 
void
test(const char* pattern, const char* path) {
  FnmatchCompiler* compiler = new FnmatchCompiler();
  compiler->Compile(pattern);
  compiler->dump();

  printf("----------------------------\n");
  printf("optimizing\n");
  compiler->optimize();
  printf("optimized\n");
  compiler->dump();

  bool result = compiler->run(path);
  delete compiler;
  bool expected = (fnmatch(pattern, path, 0) == 0);
  printf("[%c] fnmatch(\"%s\", \"%s\") = %s, expected %s\n", 
      (expected == result)?'+':'-', pattern, path, result?"true":"false", 
      expected?"true":"false");
}

int main(int argc, char**argv) {
  // important initialization - do this first
  FnmatchCompiler::Initialize();

  /*
  test("", "");
  test("?", "a");
  test("1", "a");
  test(".", "a");
  test("foo", "foo");
  */
  test("f?o", "foo");
}
