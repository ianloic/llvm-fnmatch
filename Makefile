CPPFLAGS = $(shell llvm-config --cxxflags) -Wall -Werror -g -O0
LDFLAGS = $(shell llvm-config --ldflags --libs core jit native)
LD = $(CXX)

all: fnmatch test
	
fnmatch: fnmatch.o fnmatch_parse.o
	$(CXX) -o fnmatch fnmatch.o fnmatch_parse.o $(LDFLAGS)

test: test.o
	$(CXX) -o test test.o $(LDFLAGS)

clean:
	rm -f fnmatch fnmatch.o test.o test
