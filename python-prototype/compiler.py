

from llvm import *
from llvm.core import *
from llvm.ee import *
from llvm.passes import *


def compile(dfa, name='fnmatch'):
  '''compile a deterministic finite state automaton into native code via
  llvm'''

  # create the module
  module = Module.new('fnmatch_compile')

  # character type
  char_type = Type.int(8)
  # string type (char*)
  string_type = Type.pointer(char_type)
  # boolean type
  bool_type = Type.int(1)

  # create the function i1 @fnmatch(i8*)
  function = module.add_function(
      Type.function(bool_type, [string_type]), name);
  function.args[0].name = 'path'

  # create an entry block for the function
  function_entry = function.append_basic_block('function_entry')
  entry_bb = Builder.new(function_entry)

  # create a local variable to hold the character pointer
  path_ptr = entry_bb.alloca(string_type, 'path_ptr')
  # store the %path argument
  entry_bb.store(function.args[0], path_ptr)

  # create a block that returns true
  return_true = function.append_basic_block('return_true')
  Builder.new(return_true).ret(Constant.int(bool_type, 1))

  # create a block that returns false
  return_false = function.append_basic_block('return_false')
  Builder.new(return_false).ret(Constant.int(bool_type, 0))

  # for each of the states in the NFA we create a BasicBlock
  state_blocks = {}
  for state in dfa.states:
    # create a block for this state
    state_blocks[state] = function.append_basic_block('state_block')

  # for each of the states in the NFA we add some simple code to the basic
  # blocks
  for state, block in state_blocks.items():
    bb = Builder.new(block)
    # load the path pointer
    path = bb.load(path_ptr, 'path')
    # load the character at the current point in the path
    path_char = bb.load(path, 'path_char')

    # we need to work out what goes in our big switch statement
    # find the exclusive character set (if one exists)
    exclusive = [(charset, state) for (charset, state) in state.children
        if not charset.inclusive]
    assert len(exclusive) <= 1 # zero or one only I think...
    if len(exclusive): 
      excluded_chars = exclusive[0][0].characters
      else_block = state_blocks[exclusive[0][1]]
    else: 
      excluded_chars = set()
      else_block = return_false

    # build a big-ass switch statement
    switch = bb.switch(path_char, else_block)

    for charset, block in state.children:
      if not charset.inclusive: continue # already handled
      # add a case for each character
      for c in charset.characters:
        switch.add_case(Constant.int(char_type, ord(c)), state_blocks[block])
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

    #print `state.children`

  # branch from the entry to the initial state
  entry_bb.branch(state_blocks[dfa.initial])


  print str(module)