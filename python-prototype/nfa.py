#!/usr/bin/env python

# character classes are functions f(c) -> True/False
# states are functions f(c) -> set of states

from pprint import pprint

from characterset import CharacterSet, distinctCharacterSets
from StringIO import StringIO

class NFAState:
  id = 0
  def __init__(self, name, id=None):
    self.children = []
    self.match = False
    self.name = name
    if id == None:
      self.id = NFAState.id
      NFAState.id = NFAState.id + 1
    else:
      self.id = id
  def __repr__(self):
    return 'NFAState(%s, id=%d)' % (`self.name`, self.id)
  def add(self, charset, state):
    self.children.append((charset, state))
  def __call__(self, c):
    return [x[1] for x in self.children if c in x[0]]
  def dot(self, dot):
    if self.match:
      dot.node(self.id, peripheries=2)
    else:
      dot.node(self.id)
    for charset, child in self.children:
      dot.arc(self.id, child.id, charset.label())


class NFA:
  @classmethod
  def fnmatch(klass, s):
    nfa = NFA()
    nfa.initial = NFAState('INITIAL')
    nfa.states = [nfa.initial]
    state = nfa.initial
    new_state = None
    sio = StringIO(s)
    while True:
      c = sio.read(1)
      if c == '': break # end-of-string
      new_state = NFAState(`c`)
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

  def __call__(self, s):
    states = set([self.initial])
    for c in s:
      print 'matching against %s, states: %s' % (c, `states`)
      if len(states) == 0:
        return False
      new_states = set()
      for state in states:
        new_states = new_states.union(state(c))
      states = new_states
    return len([s for s in states if s.match]) > 0

  def dot(self, dot): 
    for state in self.states:
      state.dot(dot)


