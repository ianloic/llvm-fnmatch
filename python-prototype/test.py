#!/usr/bin/env python

# character classes are functions f(c) -> True/False
# states are functions f(c) -> set of states

from pprint import pprint

from characterset import CharacterSet, distinctCharacterSets
from dfa import DFA
from nfa import NFA

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
  nfa = NFA('*.cpp')
  #nfa = NFA('a*bc')
  dfa = DFA(nfa)
  #print '-' * 40
  #pprint(dfa.states)
  print 'digraph hobo {\n\trankdir=LR\n' + nfa.dot() + dfa.dot() + '}\n'


