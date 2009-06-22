#include "fnmatch.h"
#include "characterset.h"
#include "fsm.h"
#include "nfa.h"
#include "dfa.h"

#include <fnmatch.h>

void
test(const char* pattern, const char* path) {
  FnmatchCompiler* compiler = new FnmatchCompiler();
  compiler->Compile(pattern);
  //compiler->dump();
  compiler->optimize();
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

static const char* test_patterns[] = {
  "", "?", "*", "1", ".", "foo", "f?o", "f[aeiou]o", "*.txt", "*.*",
  NULL
};

static const char* test_paths[] = {
  "", "a", "foo", "hello", "hello.txt", "hello.txto",
  NULL
};

int main(int argc, char**argv) {
  CharacterSet cs;
  NFA* nfa = NFA::fnmatch("*.txt");
  DFA dfa;

  //nfa->dot();
  //nfa = NULL;

  exit(0);

  if (argc == 3) {
    // test specific pattern and path
    test(argv[1], argv[2]);
    exit(0);
  }
  for (const char** pattern = test_patterns; *pattern; pattern++) {
    for (const char** path = test_paths; *path; path++) {
      test(*pattern, *path);
    }
  }
}
