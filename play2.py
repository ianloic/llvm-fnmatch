#!/usr/bin/env python

# character classes are functions f(c) -> True/False
# states are functions f(c) -> set of states

from pprint import pprint

from characterset import CharacterSet, distinctCharacterSets

class State:
  id = 0
  def __init__(self, name, id=None):
    self.children = []
    self.match = False
    self.name = name
    if id == None:
      self.id = State.id
      State.id = State.id + 1
    else:
      self.id = id
  def __repr__(self):
    return 'State(%s, id=%d)' % (`self.name`, self.id)
  def add(self, charset, state):
    self.children.append((charset, state))
  def __call__(self, c):
    return [x[1] for x in self.children if c in x[0]]
  def dot(self):
    return ''.join(['\t%s -> %s [label="%s"]\n' % 
      (self.id, child.id, charset.label()) for charset, child in self.children])


class NFA:
  def __init__(self, s):
    self.initial = State('INITIAL')
    self.states = [self.initial]
    state = self.initial
    new_state = None
    for c in s:
      new_state = State(`c`)
      self.states.append(new_state)
      if c == '?':
        state.add(CharacterSet(False, ''), new_state)
      elif c == '*':
        state.add(CharacterSet(False, ''), new_state)
        new_state.add(CharacterSet(False, ''), new_state)
      else:
        state.add(CharacterSet(True, c), new_state)
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


class DFAState:
  id = 0
  def __init__(self, nfa_states):
    self.children = []
    self.nfa_states = nfa_states
    self.match = False
    self.id = DFAState.id
    DFAState.id = DFAState.id + 1
  def name(self):
    return chr(65+self.id)
  def __repr__(self):
    return 'DFAState%s(%s)' % (self.name(), `self.nfa_states`)
  def addChild(self, charset, state):
    self.children.append((charset, state))
  def dot(self):
    return '\t%s [label="%s"]\n' % \
      (self.name(), ','.join([str(nfa_state.id) for nfa_state in self.nfa_states])) + \
    ''.join(['\t%s -> %s [label="%s"]\n' % \
      (self.name(), child.name(), charset.label()) for charset, child in self.children])

class DFA:
  def __init__(self, nfa):
    self.nfa = nfa
    self.states = {}
    self.__processState([nfa.initial])

  def __processState(self, nfa_states):
    # look up to see if we've processed this set of NFA states into a DFA state yet
    states_key = tuple([str(nfa_state.id) for nfa_state in nfa_states])
    if self.states.has_key(states_key):
      return self.states[states_key]
    # nope, make a new one
    dfa_state = DFAState(nfa_states)
    # stash it in the hash table
    self.states[states_key] = dfa_state

    # find all the arcs going out from the set of NFA nodes that this DFA node represents
    arcs = {}
    for nfa_state in nfa_states:
      for charset, child_state in nfa_state.children:
        if not arcs.has_key(charset):
          arcs[charset] = []
        arcs[charset].append(child_state)

    # make the arcs "distinct"
    arcs = distinctArcs(arcs)

    for charset, child_nfa_states in arcs.items():
      dfa_state.addChild(charset, self.__processState(child_nfa_states))

    return dfa_state

  def dot(self): 
    return ''.join([state.dot() for junk, state in self.states.items()])

def distinctArcs(arcs):
  '''for a dict of arcs { charset->(state,state) } produce a new dict 
  { charset->(state, state) } that represents an equivalent mapping
  but new charsets form a partition of the union of the original
  charsets, where each of the new sets is a subset of one or more of
  the old sets'''

  #print 'distinctArcs(%s)' % `arcs`

  # nothing to do with one or zero children
  if len(arcs) < 2:
    return arcs

  partition = distinctCharacterSets(arcs.keys())

  #print ' partition: %s' % `partition`

  # now we have to stick the charsets back with the appropriate states
  charsets = {}
  for charset in partition:
    charsets[charset] = set()
    for original_charset, states in arcs.items():
      if (charset - original_charset).empty():
        # charset is a subset of original_charset
        charsets[charset] = charsets[charset].union(set(states))

  #print ' charsets: %s' % `charsets`

  # now, if we have multiple charsets going to the same set of states we
  # should collapse the charsets (ie: union)
  charset_by_states_key = {}
  states_by_states_key = {}
  for charset, states in charsets.items():
    states_key = tuple([str(state.id) for state in states])
    states_by_states_key[states_key] = states
    if charset_by_states_key.has_key(states_key):
      charset_by_states_key[states_key] = charset_by_states_key[states_key].union(charset)
    else:
      charset_by_states_key[states_key] = charset

  # connect those mappings together
  result = {}
  for states_key, charset in charset_by_states_key.items():
    result[charset] = states_by_states_key[states_key]

  # check that our result matches our contract
  # make sure that none of our character sets intersect
  union_out = CharacterSet(True, '') # empty set
  for cs in result.keys():
    assert cs.disjoint(union_out)
    union_out = union_out.union(cs)
  # make sure that the union of result character sets == the union of the input character sets
  union_in = reduce(lambda a,b:a.union(b), arcs.keys())
  assert union_in == union_out

  return result


def test_distinctArcs():
  state1 = State('a')
  state2 = State('b')
  state3 = State('c')
  assert distinctArcs({}) == {}
  assert distinctArcs({CharacterSet(False, ''): [state1]}) == {CharacterSet(False, ''): [state1]}
  assert distinctArcs({CharacterSet(True, 'abc'): [state1], CharacterSet(True, 'def'): [state2]}) == \
      {CharacterSet(True, 'abc'): set([state1]), CharacterSet(True, 'def'): set([state2])}
  assert distinctArcs({CharacterSet(False, ''): [state1], CharacterSet(True, 'abc'): [state2]}) == \
      {CharacterSet(False, 'abc'): set([state1]), CharacterSet(True, 'abc'): set([state1,state2])}


if __name__ == '__main__':
  from characterset import test_CharacterSet
  test_CharacterSet()

  #test_distinctArcs()

#  print "CharacterSet(False, '') - CharacterSet(True, '.') = " + `CharacterSet(False, '') - CharacterSet(True, '.')`
#  print NFA('*.txt').dot()
#  print `NFA('*.txt')('hello.txt')`
#  print `NFA('*.txt')('hello.txto')`
#  print `NFA('*.txt')('hello.txt.txt')`

  #nfa = NFA('*.txt')
  nfa = NFA('a*b*.txt')
  #nfa = NFA('a*bc')
  dfa = DFA(nfa)
  #print '-' * 40
  #pprint(dfa.states)
  print 'digraph hobo {\n\trankdir=LR\n' + nfa.dot() + dfa.dot() + '}\n'


