#!/usr/bin/env python

# character classes are functions f(c) -> True/False
# states are functions f(c) -> set of states

from pprint import pprint

from dfa import DFA
from nfa import NFA

from dot import Dot

if __name__ == '__main__':
  from characterset import test_CharacterSet
  test_CharacterSet()

  #test_distinctArcs()

#  print "CharacterSet.excluding('') - CharacterSet.including('.') = " + `CharacterSet.excluding('') - CharacterSet.including('.')`
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

  dot = Dot('hobo')
  nfa.dot(dot)
  dfa.dot(dot)
  dot.show()
  #print str(dot)

