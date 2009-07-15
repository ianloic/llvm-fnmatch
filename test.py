#!/usr/bin/env python

# our code
from dfa import DFA
from nfa import NFA
from compiler import Compiled

# the python implementation
import fnmatch

# run unit self tests
from characterset import test_CharacterSet
test_CharacterSet()
from dfa import test_distinctArcs
test_distinctArcs()

PATTERNS = ('*.txt', '*', '*.*', 'README.*')
PATHS = ('test.c', 'README.txt', 'README')

for pattern in PATTERNS:
  nfa = NFA.fnmatch(pattern)
  dfa = DFA(nfa)
  compiled = Compiled(dfa, debug=False)
  compiled.optimize()
  for path in PATHS:
    expected = fnmatch.fnmatch(path, pattern)
    assert nfa(path) == expected
    assert dfa(path) == expected
    assert compiled(path) == expected

