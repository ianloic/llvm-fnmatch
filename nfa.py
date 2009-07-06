#!/usr/bin/env python

# character classes are functions f(c) -> True/False
# states are functions f(c) -> set of states

from StringIO import StringIO

from characterset import CharacterSet, distinctCharacterSets
from fsm import State, StateMachine

class NFAState(State):
  pass


class NFA(StateMachine):
  @classmethod
  def fnmatch(klass, s):
    nfa = NFA(NFAState())
    state = nfa.initial
    new_state = None
    sio = StringIO(s)
    while True:
      c = sio.read(1)
      if c == '': break # end-of-string
      new_state = NFAState()
      nfa.states.append(new_state)
      if c == '?':
        # single-character wildcard
        state.add(CharacterSet.excluding(''), new_state)
      elif c == '*':
        # multi-character wildcard
        state.add(CharacterSet.excluding(''), new_state)
        new_state.add(CharacterSet.excluding(''), new_state)
      elif c == '[':
        # bracket expression
        c = sio.read(1)
        if c == '!': # inverted
          inverted = True
          c = sio.read(1)
        else:
          inverted = False
        charset = CharacterSet.including('')
        last_char = None
        while c != ']':
          if c == '':
            raise 'unterminated bracket expression'
          if c == '-' and last_char:
            c = sio.read(1)
            charset = charset.union(CharacterSet.range(last_char, c))
            last_char = None
            c = sio.read(1)
            continue
          charset = charset.union(CharacterSet.including(c))
          last_char = c # save last character
          c = sio.read(1)
        if inverted:
          state.add(CharacterSet.excluding('') - charset, new_state)
        else:
          state.add(charset, new_state)
      else:
        state.add(CharacterSet.including(c), new_state)
      state = new_state
    state.match = True
    return nfa

  def __init__(self, initial, states=[]):
    StateMachine.__init__(self, initial, states)

  def dot(self, dot): 
    for state in self.states:
      state.dot(dot)


