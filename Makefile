CPPFLAGS = $(shell llvm-config --cxxflags)
LDFLAGS = $(shell llvm-config --ldflags) $(shell llvm-config --libs core jit native)

all: fnmatch
	
fnmatch: fnmatch.o
	$(CXX) -o fnmatch fnmatch.o $(LDFLAGS)
