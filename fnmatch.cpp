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


FnmatchFunction::FnmatchFunction(FnmatchCompiler* _compiler,
    FnmatchRules::const_iterator& _rule_iter, 
    const FnmatchRules::const_iterator& _rule_end, 
    const std::string& name) 
    : compiler(_compiler), rule_iter(_rule_iter), rule_end(_rule_end) {

  // create a function of type i1(char*)
  std::vector<const Type*> args_type;
  args_type.push_back(PointerType::get(IntegerType::get(8), 0));
  FunctionType *func_type = FunctionType::get(IntegerType::get(1), args_type, false);
  func = Function::Create(func_type, Function::ExternalLinkage, name, 
      FnmatchCompiler::module);

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

  BasicBlock* pre = NULL;
  BasicBlock* post = entry;

  while (rule_iter < rule_end) {
    // create a block to begin this rule
    pre = BasicBlock::Create("pre", func);
    // wire the last rule to this rule
    if (post) {
      BranchInst::Create(pre, post);
    }
    // create a block to end the rule
    post = BasicBlock::Create("post", func);
    // get the rule
    FnmatchRule* rule = *rule_iter;
    // compile the rule
    rule->Compile(this, pre, post);
    // next rule...
    rule_iter++;
  }

  // okay, everything matched, return true
  BranchInst::Create(return_true, post);

  // check the function, eh?
  verifyFunction(*func);
}


void
FnmatchCompiler::Compile(const std::string& pattern, const std::string& name) {
  // parse the fnmatch expression into rules
  FnmatchParser parser;
  FnmatchRules rules = parser.Parse(pattern);

  FnmatchRules::const_iterator rule_iter = rules.begin();
  FnmatchRules::const_iterator rule_end = rules.end();

  function = new FnmatchFunction(this, rule_iter, rule_end, name);
}

Value* 
FnmatchFunction::loadPathCharacter(BasicBlock* basicBlock) {
  // load the index
  Value* index = new LoadInst(index_ptr, "index", basicBlock);
  // get a pointer to the path character
  GetElementPtrInst* path_char_ptr = GetElementPtrInst::Create(path_ptr, index, "", basicBlock);
  // load the value
  LoadInst* path_char = new LoadInst(path_char_ptr, "path_char", basicBlock);
  return path_char;
}

void
FnmatchFunction::consumePathCharacter(BasicBlock* basicBlock) {
  // load the index
  Value* index = new LoadInst(index_ptr, "index", basicBlock);
  // increment
  index = BinaryOperator::CreateAdd(index, ConstantInt::get(IntegerType::get(32), 1), "increment", basicBlock);
  // store the index
  new StoreInst(index, index_ptr, basicBlock);
}

void
FnmatchCompiler::optimize() {
  fpm->run(*function->func);
}

bool
FnmatchCompiler::run(const char* path) {
  // okay, let's run this thing
  void* func_void_ptr = executionEngine->getPointerToFunction(function->func);
  bool (*func_ptr)(const char*) = (bool (*)(const char*))func_void_ptr;
  return (func_ptr)(path);
}




void
FnmatchCharacter::Compile(FnmatchFunction* compiler, BasicBlock* pre, BasicBlock* post) {
  // load the path character
  Value* path_char = compiler->loadPathCharacter(pre);
  // consume the path character
  compiler->consumePathCharacter(pre);
  // compare the path character with the rule character
  ICmpInst* cmp_chr = new ICmpInst(ICmpInst::ICMP_NE, path_char, 
      ConstantInt::get(IntegerType::get(8), character), "", pre);
  // branch
  BranchInst::Create(compiler->return_false, post, cmp_chr, pre);
}


void
FnmatchSingle::Compile(FnmatchFunction* compiler, BasicBlock* pre, BasicBlock* post) {
  // match any character, so just check against \0
  Value* path_char = compiler->loadPathCharacter(pre);
  compiler->consumePathCharacter(post);
  ICmpInst* cmp_chr = new ICmpInst(ICmpInst::ICMP_EQ, path_char, 
      ConstantInt::get(IntegerType::get(8), 0), "", pre);
  BranchInst::Create(compiler->return_false, post, cmp_chr, pre);
}

void
FnmatchBracket::Compile(FnmatchFunction* compiler, BasicBlock* pre, BasicBlock* post) {
  // load the path character
  Value* path_char = compiler->loadPathCharacter(pre);
  // consume the path character
  compiler->consumePathCharacter(pre);

  BasicBlock* matched;
  BasicBlock* unmatched;

  if (inverse) {
    // any match -> return false
    matched = compiler->return_false;
    // no match -> continue
    unmatched = post;
  } else {
    // any match -> continue
    matched = post;
    // no match -> return false
    unmatched = compiler->return_false;
  }

  // switch on the path character
  SwitchInst* sw = SwitchInst::Create(path_char, unmatched, 
      characters.size(), pre);
  // for each character in the set...
  for (std::string::const_iterator i = characters.begin();
      i < characters.end(); i++) {
    sw->addCase(ConstantInt::get(IntegerType::get(8), *i), matched);
  }
}



void
FnmatchMultiple::Compile(FnmatchFunction* function, BasicBlock* pre, BasicBlock* post) {
  /*
  FnmatchFunction* sub_function = new FnmatchFunction(function->compiler,
      function->rule_iter, function->rule_end, "sub");
      */
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
  test("f?o", "foo");
  */
  test("f[aeiou]o", "foo");
}
