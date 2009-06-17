#!/usr/bin/env python

# character classes are functions f(c) -> True/False
# states are functions f(c) -> set of states

from pprint import pprint

from characterset import CharacterSet, distinctCharacterSets

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
  def dot(self):
    return ''.join(['\t%s -> %s [label="%s"]\n' % 
      (self.id, child.id, charset.label()) for charset, child in self.children])


class NFA:
  def __init__(self, s):
    self.initial = NFAState('INITIAL')
    self.states = [self.initial]
    state = self.initial
    new_state = None
    for c in s:
      new_state = NFAState(`c`)
      self.states.append(new_state)
      if c == '?':
        state.add(CharacterSet.excluding(''), new_state)
      elif c == '*':
        state.add(CharacterSet.excluding(''), new_state)
        new_state.add(CharacterSet.excluding(''), new_state)
      else:
        state.add(CharacterSet.including(c), new_state)
      state = new_state
    state.match = True

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

  def dot(self): 
    return ''.join([state.dot() for state in self.states])


