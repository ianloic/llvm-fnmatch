

from llvm import *
from llvm.core import *
from llvm.ee import *
from llvm.passes import *


class Compiled:
  ee = None
  def __init__(self, dfa, debug=True):
    '''compile a deterministic finite state automaton into native code via
    llvm'''

    # create the module
    if debug:
      from StringIO import StringIO
      self.module = Module.from_assembly(StringIO(
        '''declare i32 @putchar(i32) nounwind'''))
      putchar = self.module.get_function_named('putchar')
    else:
      self.module = Module.new('fnmatch_compile')
    if not Compiled.ee:
      Compiled.ee = ExecutionEngine.new(ModuleProvider.new(self.module))

    # character type
    char_type = Type.int(8)
    # string type (char*)
    string_type = Type.pointer(char_type)
    # boolean type
    bool_type = Type.int(1)

    # create the function i1 @fnmatch(i8*)
    self.function = self.module.add_function(
        Type.function(bool_type, [string_type]), 'fnmatch');
    self.function.args[0].name = 'path'

    # create an entry block for the function
    function_entry = self.function.append_basic_block('function_entry')
    entry_bb = Builder.new(function_entry)

    # create a local variable to hold the character pointer
    path_ptr = entry_bb.alloca(string_type, 'path_ptr')
    # store the %path argument
    entry_bb.store(self.function.args[0], path_ptr)

    # create a block that returns true
    return_true = self.function.append_basic_block('return_true')
    Builder.new(return_true).ret(Constant.int(bool_type, 1))

    # create a block that returns false
    return_false = self.function.append_basic_block('return_false')
    Builder.new(return_false).ret(Constant.int(bool_type, 0))

    # for each of the states in the NFA we create a BasicBlock
    for state in dfa.states:
      state.block = self.function.append_basic_block('state_'+state.name)

    # for each of the states in the NFA we add some simple code to the basic
    # blocks
    for state in dfa.states:
      bb = Builder.new(state.block)
      # load the path pointer
      path = bb.load(path_ptr, 'path')
      # load the character at the current point in the path
      path_char = bb.load(path, 'path_char')
      # increment the path pointer
      path_int = bb.ptrtoint(path, Type.int(32))
      path_int = bb.add(path_int, Constant.int(Type.int(32), 1))
      path = bb.inttoptr(path_int, string_type)
      bb.store(path, path_ptr)

      if debug:
        bb.call(putchar, [Constant.int(Type.int(32), ord(state.name))])
        bb.call(putchar, [bb.zext(path_char, Type.int(32))])
        bb.call(putchar, [Constant.int(Type.int(32), ord('\n'))])

      # we need to work out what goes in our big switch statement
      # find the exclusive character set (if one exists)
      exclusive = [(c, s) for (c, s) in state if not c.inclusive]
      assert len(exclusive) <= 1 # zero or one only I think...
      if len(exclusive): 
        excluded_chars = exclusive[0][0].characters
        else_block = exclusive[0][1].block
      else: 
        excluded_chars = set()
        else_block = return_false

      # build a big-ass switch statement
      switch = bb.switch(path_char, else_block)

      for charset, child in state:
        if not charset.inclusive: continue # already handled
        # add a case for each character
        for c in charset.characters:
          switch.add_case(Constant.int(char_type, ord(c)), child.block)
        # remove the handled chars from the excluded chars
        excluded_chars = excluded_chars - charset.characters
      # if there are any excluded characters left over, return false for them
      if excluded_chars:
        for c in excluded_chars:
          switch.add_case(Constant.int(char_type, ord(c)), return_false)

      # handle end of string '\0'
      if state.match:
        # end of string in a match state -> return true
        switch.add_case(Constant.int(char_type, 0), return_true)
      else:
        # end of string elsewhere -> return false
        switch.add_case(Constant.int(char_type, 0), return_false)

    # branch from the entry to the initial state
    entry_bb.branch(dfa.initial.block)

  def __str__(self):
    return str(self.module)

  def __call__(self, path):
    path_value = GenericValue.string(Type.pointer(Type.int(8)), path)
    retval = Compiled.ee.run_function(self.function, [path_value])
    return (retval.as_int() != 0)

  def optimize(self, level=2):
    '''run the bitcode though the LLVM opt program'''
    from subprocess import Popen, PIPE
    opt = Popen(('opt', '-O%d' % level), stdin=PIPE, stdout=PIPE)
    self.module.to_bitcode(opt.stdin)
    opt.stdin.close()
    self.module = Module.from_bitcode(opt.stdout)
    self.function = self.module.get_function_named('fnmatch')

