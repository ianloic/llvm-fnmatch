#!/usr/bin/env python

# character classes are functions f(c) -> True/False
# states are functions f(c) -> set of states


from dfa import DFA
from nfa import NFA

from dot import Dot

if __name__ == '__main__':
  from characterset import test_CharacterSet
  test_CharacterSet()

  #from dfa import test_distinctArcs
  #test_distinctArcs()

#  print "CharacterSet.excluding('') - CharacterSet.including('.') = " + `CharacterSet.excluding('') - CharacterSet.including('.')`
#  print NFA.fnmatch('*.txt').dot()
#  print `NFA.fnmatch('*.txt')('hello.txt')`
#  print `NFA.fnmatch('*.txt')('hello.txto')`
#  print `NFA.fnmatch('*.txt')('hello.txt.txt')`

  #nfa = NFA.fnmatch('*.txt')
  nfa = NFA.fnmatch('*.cpp')
  #nfa = NFA.fnmatch('[a-zA-Z1-90]*.cpp')
  #nfa = NFA.fnmatch('a*bc')
  dfa = DFA(nfa)

  dot = Dot('hobo')
  dot.add(nfa)
  dot.add(dfa)
  #dot.show()

  from compiler import Compiled
  compiled = Compiled(dfa, debug=False)
  compiled.optimize()

  result = compiled('test.cpp')
  print 'result=%s' % `result`

  result = compiled('test.c')
  print 'result=%s' % `result`

  compiled2 = Compiled(DFA(nfa.fnmatch('hello*')), debug=False)
  compiled2('hello world')
  compiled2('sucker hello')
