CPPFLAGS = $(shell llvm-config --cxxflags) -Wall -Werror
LDFLAGS = $(shell llvm-config --ldflags --libs core jit native)

all: fnmatch
	
fnmatch: fnmatch.o
	$(CXX) -o fnmatch fnmatch.o $(LDFLAGS)

clean:
	rm -f fnmatch fnmatch.o
