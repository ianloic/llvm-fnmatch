#include "fnmatch.h"

#include <fnmatch.h>
#include <sys/time.h>

#define PATTERN "*.*"
#define PATH "hello.txt"
//#define RUNS 1000000
#define RUNS 10000000

class Timer {
  public:
    void start() { gettimeofday(&_start, NULL); }
    void stop() { gettimeofday(&_stop, NULL); }
    double seconds() {
      return (double)(_stop.tv_sec-_start.tv_sec) + 
        (double)(_stop.tv_usec-_start.tv_usec) / (double)1000000;
    }
  private:
    struct timeval _start;
    struct timeval _stop;
};

int main(int argc, char**argv) {
  Timer setup;
  setup.start();

  FnmatchCompiler* compiler = new FnmatchCompiler();
  compiler->Compile(PATTERN);
  compiler->optimize();
  setup.stop();

  Timer llvm_impl, libc_impl;
  int i;

  llvm_impl.start();
  for (i=0; i<RUNS; i++) {
    compiler->run(PATH);
  }
  llvm_impl.stop();

  libc_impl.start();
  for (i=0; i<RUNS; i++) {
    fnmatch(PATTERN, PATH, 0);
  }
  libc_impl.stop();

  printf ("testing %d times...\n", RUNS);
  printf ("llvm setup took %lf seconds\n", setup.seconds());
  printf ("llvm runs took %lf seconds\n", llvm_impl.seconds());
  printf ("libc runs took %lf seconds\n", libc_impl.seconds());

  //compiler->dump();
}

