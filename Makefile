CPPFLAGS = $(shell llvm-config --cxxflags) -Wall -Werror -g -O0
LDFLAGS = $(shell llvm-config --ldflags --libs core jit native)
LD = $(CXX)

all: fnmatch-test
	
fnmatch-test: fnmatch-test.o fnmatch.o fnmatch-parse.o
	$(CXX) -o fnmatch-test fnmatch-test.o fnmatch.o fnmatch-parse.o $(LDFLAGS)

clean:
	rm -f fnmatch-test fnmatch.o fnmatch-parse.o
