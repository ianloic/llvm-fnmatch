#!/usr/bin/env python


from dfa import DFA
from nfa import NFA
from compiler import Compiled

from datetime import datetime

from llvm import *
from llvm.core import *
from llvm.ee import *
from llvm.passes import *

def compile_native_test(compiled, pattern, path, count):
  # okay, let's add a function that runs the test
  count_type = Type.int(32)
  loop_function = compiled.module.add_function(Type.function(Type.void(), []), 
      'native_test')
  entry = loop_function.append_basic_block('entry')
  bb = Builder.new(entry)

  # local variable for the counter
  count_ptr = bb.alloca(count_type, 'count_ptr')
  bb.store(Constant.int(count_type, count), count_ptr)

  # local variable for the path
  path_arr = bb.alloca(Type.array(Type.int(8), len(path)+1), 'path_arr')
  bb.store(Constant.stringz(path), path_arr)
  path_ptr = bb.gep(path_arr, 
      [Constant.int(count_type, 0), Constant.int(count_type, 0)])

  # create a function exit block
  exit = loop_function.append_basic_block('exit')
  Builder.new(exit).ret_void()

  # create a loop block
  loop = loop_function.append_basic_block('loop')
  bb.branch(loop)
  bb = Builder.new(loop)
  # call the compiled function
  bb.call(compiled.function, [path_ptr])
  # decrement the counter
  count = bb.load(count_ptr, 'count')
  count = bb.sub(count, Constant.int(count_type, 1))
  bb.store(count, count_ptr)
  # loop if we've got more to do...
  zero = bb.icmp(ICMP_EQ, count, Constant.int(count_type, 0))
  bb.cbranch(zero, exit, loop)

  return lambda ee=Compiled.ee,func=loop_function:ee.run_function(func, [])
 

def test(pattern, path, count):
  print 'fnmatch(%s, %s) %d times...' % (path, pattern, count)
  start_compile = datetime.now()
  compiled = Compiled(DFA(NFA.fnmatch(pattern)))
  compiled.optimize() # slow!
  end_compile = datetime.now()

  start_execution = datetime.now()
  for x in range(count):
    compiled(path)
  end_execution = datetime.now()

  native_test = compile_native_test(compiled, pattern, path, count)

  start_native = datetime.now()
  native_test()
  end_native = datetime.now()

  from fnmatch import fnmatch

  start_builtin = datetime.now()
  for x in range(count):
    fnmatch(path, pattern)
  end_builtin = datetime.now()

  print 'compile time:   %s' % (end_compile-start_compile)
  print 'execution time: %s' % (end_execution-start_execution)
  print 'native time:    %s' % (end_native-start_native)
  print 'builtin time:   %s' % (end_builtin-start_builtin)
  print ''

TIMES = 10000

test('*.txt', 'README.txt', TIMES)
test('*.txt', 'README', TIMES)

test('README*', 'README.txt', TIMES)
test('README*', 'README', TIMES)
test('README', 'README', TIMES)

