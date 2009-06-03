CPPFLAGS = $(shell llvm-config --cxxflags) -Wall -Werror -g -O0
LDFLAGS = $(shell llvm-config --ldflags --libs core jit native)
LD = $(CXX)

all: fnmatch-test fnmatch-benchmark
	
fnmatch-test: fnmatch-test.o fnmatch-compiler.o fnmatch-parse.o
	$(CXX) -o fnmatch-test fnmatch-test.o fnmatch-compiler.o fnmatch-parse.o $(LDFLAGS)

fnmatch-benchmark: fnmatch-benchmark.o fnmatch-compiler.o fnmatch-parse.o
	$(CXX) -o fnmatch-benchmark fnmatch-benchmark.o fnmatch-compiler.o fnmatch-parse.o $(LDFLAGS)

clean:
	rm -f fnmatch-test fnmatch-benchmark fnmatch-test.o fnmatch-benchmark.o fnmatch-compiler.o fnmatch-parse.o
