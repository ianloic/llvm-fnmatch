#include "fnmatch.h"

#include <fnmatch.h>

void
test(const char* pattern, const char* path) {
  FnmatchCompiler* compiler = new FnmatchCompiler();
  compiler->Compile(pattern);
  //compiler->dump();
  //compiler->optimize();
  //compiler->dump();

  bool result = compiler->run(path);
  delete compiler;
  bool expected = (fnmatch(pattern, path, 0) == 0);
  printf("[%c] fnmatch(\"%s\", \"%s\") = %s, expected %s\n", 
      (expected == result)?'+':'-', pattern, path, result?"true":"false", 
      expected?"true":"false");
  if (expected != result) {
    // our algorithm didn't match system fnmatch, dump & fail
    compiler->dump();
    exit(1);
  }

  // done testing, throw the function away
  //compiler->reset();
}

int main(int argc, char**argv) {
  // important initialization - do this first
  FnmatchCompiler::Initialize();

  test("", "");
  test("", "a");
  test("?", "a");
  test("1", "a");
  test(".", "a");
  test("foo", "foo");
  test("f?o", "foo");
  test("f[aeiou]o", "foo");
  test("*.txt", "hello");
  test("*.txt", "hello.txto");
}
