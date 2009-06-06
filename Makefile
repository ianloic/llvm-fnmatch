# using autodeps code from http://www.cs.berkeley.edu/~smcpeak/autodepend/autodepend.html
# use gnu make and gcc. duh.

CPPFLAGS = $(shell llvm-config --cxxflags) -Wall -Werror -g -O0
LDFLAGS = $(shell llvm-config --ldflags --libs core jit native)
LD = $(CXX)

BASE_OBJS = fnmatch-compiler.o fnmatch-parse.o
PROG_OBJS = fnmatch-test.o fnmatch-benchmark.o fnmatch-statemachine.o
OBJS = $(BASE_OBJS) $(PROG_OBJS)

PROGRAMS=fnmatch-test fnmatch-benchmark fnmatch-statemachine

# build some programs
all: $(PROGRAMS)

# pull in dependency info for *existing* .o files
-include $(OBJS:.o=.d)

# rebuild when the Makefile changes
$(OBJS): Makefile

# compile and generate dependency info
%.o: %.cpp
	$(CXX) -c $(CPPFLAGS) $*.cpp -o $*.o
	$(CXX) -MM $(CPPFLAGS) $*.cpp > $*.d
	@cp -f $*.d $*.d.tmp
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp


%: %.o $(BASE_OBJS)
	$(CXX) -o $* $*.o $(BASE_OBJS) $(LDFLAGS)

clean:
	rm -f $(PROGRAMS) $(OBJS)
