#!/usr/bin/env python

# character classes are functions f(c) -> True/False
# states are functions f(c) -> set of states

from pprint import pprint

class State:
  id = 0
  def __init__(self, name):
    self.children = []
    self.match = False
    self.name = name
    self.id = State.id
    State.id = State.id + 1
  def __repr__(self):
    return 'State%d(%s)' % (self.id, self.name)
  def add(self, charset, state):
    self.children.append((charset, state))
  def __call__(self, c):
    return [x[1] for x in self.children if c in x[0]]
  def dot(self):
    return ''.join(['\t%s -> %s [label="%s"]\n' % 
      (self.id, child.id, charset.label()) for charset, child in self.children])

  def make_distinct_children(self):
    '''split the children so that the character sets overlap completely or not at all'''
    # nothing to do with one or zero children
    if len(self.children) < 2:
      return
    assert len(self.children) == 2
    sets_a, sets_b = self.children[0][0].distinct(self.children[1][0])
    dc = []
    for charset in sets_a:
      dc.append((charset, self.children[0][1]))
    for charset in sets_b:
      dc.append((charset, self.children[1][1]))
    self.children = [child for child in dc if not child[0].none()]


class CharacterSet:
  def __init__(self, inclusive, characters):
    self.inclusive = bool(inclusive)
    self.characters = set(characters)
  def __contains__(self, c):
    if self.inclusive:
      return c in self.characters
    else:
      return not (c in self.characters)
  def label(self):
    if self.inclusive:
      return `''.join(self.characters)`
    else:
      if not self.characters: return 'ANY'
      else: return '!'+`''.join(self.characters)`
  def __eq__(self, other):
    return self.inclusive == other.inclusive and \
        self.characters == other.characters
  def __ne__(self, other):
    return not (self == other)
  def __hash__(self):
    return hash('%s %s' % (self.inclusive, ''.join(self.characters)))

  def union(self, other):
    if self.inclusive == other.inclusive:
      return CharacterSet(self.inclusive, ''.join(self.characters.union(other.characters)))
    elif self.inclusive:
      return CharacterSet(False, ''.join(other.characters-self.characters))
    else:
      return CharacterSet(False, ''.join(self.characters-other.characters))

  def __sub__(self, other):
    if self.inclusive and other.inclusive:
      return CharacterSet(True, ''.join(self.characters-other.characters))
    elif not self.inclusive and not other.inclusive:
      return CharacterSet(True, ''.join(other.characters-self.characters))
    elif self.inclusive and not other.inclusive:
      return CharacterSet(True, ''.join(self.characters.intersection(other.characters)))
    elif not self.inclusive and other.inclusive:
      return CharacterSet(False, ''.join(self.characters.union(other.characters)))
    else:
      raise 'wtf?'

  def intersection(self, other):
    return self - (self - other)

  def all(self):
    '''matches all characters'''
    return not self.inclusive and not self.characters

  def none(self):
    '''matches no characters'''
    return self.inclusive and not self.characters

  def __repr__(self):
    return 'CharacterSet(%s, %s)' % (`self.inclusive`, `''.join(self.characters)`)

  def distinct(self, other):
    '''return pair of tuples of sets. none of the returned sets intersect,
    the union of each pair is equal to self or other, respectively'''
    return ((self-other, self.intersection(other)), (other-self, self.intersection(other)))


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
    return 'digraph NFA {\n\trankdir=LR\n' + \
        ''.join([state.dot() for state in self.states]) + \
        '}\n'


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
    return ''.join(['\t%s -> %s [label="%s"]\n' % 
      (self.name(), child.name(), charset.label()) for charset, child in self.children])


class DFA:
  def __init__(self, nfa):
    self.nfa = nfa
    for nfa_state in nfa.states:
      nfa_state.make_distinct_children()
    self.states = {}
    self.__processState([nfa.initial])
#    for state in nfa.states:
#      print 'processing: ' + `state`
#      state.make_distinct_children()
#      dfas = {}
#      for charset, child_state in state.children:
#        if not dfas.has_key(charset):
#          dfas[charset] = []
#        dfas[charset].append(child_state)
#      for charset, nfa_states in dfas:
#        dfa_state = self.getState(nfa_states)

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

    #pprint(arcs)

    # FIXME: need to make all arc charsets distinct (since they may be from multiple source arcs)

    for charset, child_nfa_states in arcs.items():
      dfa_state.addChild(charset, self.__processState(child_nfa_states))

    return dfa_state

    

      
#  def getState(self, nfa_states):
#    states_key = ','.join([str(nfa_state.id) for nfa_state in nfa_states])
#    if not self.states.has_key(states_key):
#      self.states[states_key] = DFAState(nfa_states)
#    return self.states[states_key]

  def dot(self):
    return 'digraph DFA {\n\trankdir=LR\n' + \
        ''.join([state.dot() for junk, state in self.states.items()]) + \
        '}\n'


def test_CharacterSet():
  # test .all() and .none()
  assert CharacterSet(False, '').all()
  assert CharacterSet(True, '').none()
 
  # test that character order is irrelevant
  assert CharacterSet(True, 'ab') == CharacterSet(True, 'ba')
  assert CharacterSet(False, 'ab') == CharacterSet(False, 'ba')

  # test equality and inequality with varying inclusivity
  assert CharacterSet(True, 'ab') == CharacterSet(True, 'ab')
  assert (CharacterSet(True, 'ab') != CharacterSet(True, 'ab')) == False
  assert CharacterSet(True, 'ab') != CharacterSet(False, 'ab')
  assert (CharacterSet(True, 'ab') == CharacterSet(False, 'ab')) == False

  # test union with inclusive character sets
  assert CharacterSet(True, 'ab').union(CharacterSet(True, 'bc')) == CharacterSet(True, 'abc')
  assert CharacterSet(True, 'ab').union(CharacterSet(True, 'cd')) == CharacterSet(True, 'abcd')

  # test union with inclusive and exclusive character sets
  assert CharacterSet(True, 'ab').union(CharacterSet(False, '')) == CharacterSet(False,'')
  assert CharacterSet(True, 'ab').union(CharacterSet(False, 'cd')) == CharacterSet(False,'cd')
  assert CharacterSet(True, 'ab').union(CharacterSet(False, 'bc')) == CharacterSet(False,'c')
  assert CharacterSet(False, '').union(CharacterSet(True, 'ab')) == CharacterSet(False,'')
  assert CharacterSet(False, 'cd').union(CharacterSet(True, 'ab')) == CharacterSet(False,'cd')
  assert CharacterSet(False, 'bc').union(CharacterSet(True, 'ab')) == CharacterSet(False,'c')

  # test union with exclusive character sets
  assert CharacterSet(False, 'ab').union(CharacterSet(False, 'bc')) == CharacterSet(False, 'abc')
  assert CharacterSet(False, 'ab').union(CharacterSet(False, 'cd')) == CharacterSet(False, 'abcd')

  # test difference with inclusive character sets
  assert CharacterSet(True, 'ab') - CharacterSet(True, 'bc') == CharacterSet(True, 'a')
  assert CharacterSet(True, 'ab') - CharacterSet(True, 'cd') == CharacterSet(True, 'ab')

  # test difference with inclusive and exclusive character sets
  assert CharacterSet(True, 'ab') - CharacterSet(False, '') == CharacterSet(True,'')
  assert CharacterSet(True, 'ab') - CharacterSet(False, 'cd') == CharacterSet(True,'')
  assert CharacterSet(True, 'ab') - CharacterSet(False, 'bc') == CharacterSet(True,'b')
  assert CharacterSet(False, '') - CharacterSet(True, 'ab') == CharacterSet(False,'ab')
  assert CharacterSet(False, 'cd') - CharacterSet(True, 'ab') == CharacterSet(False,'abcd')
  assert CharacterSet(False, 'bc') - CharacterSet(True, 'ab') == CharacterSet(False,'abc')
  assert CharacterSet(True, '') - CharacterSet(False, '') == CharacterSet(True, '')
  assert CharacterSet(False, '') - CharacterSet(True, '') == CharacterSet(False, '')

  # test difference with exclusive character sets
  assert CharacterSet(False, 'ab') - CharacterSet(False, 'bc') == CharacterSet(True, 'c')
  assert CharacterSet(False, 'ab') - CharacterSet(False, 'cd') == CharacterSet(True, 'cd')

  # test intersection with inclusive character sets
  assert CharacterSet(True, 'ab').intersection(CharacterSet(True, 'bc')) == CharacterSet(True, 'b')
  assert CharacterSet(True, 'ab').intersection(CharacterSet(True, 'cd')) == CharacterSet(True, '')

  # test intersection with inclusive and exclusive character sets
  assert CharacterSet(True, 'ab').intersection(CharacterSet(False, '')) == CharacterSet(True, 'ab')
  assert CharacterSet(True, 'ab').intersection(CharacterSet(False, 'cd')) == CharacterSet(True, 'ab')
  assert CharacterSet(True, 'ab').intersection(CharacterSet(False, 'bc')) == CharacterSet(True, 'a')
  assert CharacterSet(False, '').intersection(CharacterSet(True, 'ab')) == CharacterSet(True, 'ab')
  assert CharacterSet(False, 'cd').intersection(CharacterSet(True, 'ab')) == CharacterSet(True, 'ab')
  assert CharacterSet(False, 'bc').intersection(CharacterSet(True, 'ab')) == CharacterSet(True, 'a')
  assert CharacterSet(False, '').intersection(CharacterSet(True, '')) == CharacterSet(True, '')
  assert CharacterSet(True, '').intersection(CharacterSet(False, '')) == CharacterSet(True, '')




if __name__ == '__main__':
  test_CharacterSet()
#  print "CharacterSet(False, '') - CharacterSet(True, '.') = " + `CharacterSet(False, '') - CharacterSet(True, '.')`
#  print NFA('*.txt').dot()
#  print `NFA('*.txt')('hello.txt')`
#  print `NFA('*.txt')('hello.txto')`
#  print `NFA('*.txt')('hello.txt.txt')`

  nfa = NFA('a*b*.txt')
  dfa = DFA(nfa)
  #print '-' * 40
  #pprint(dfa.states)
  print dfa.dot()


