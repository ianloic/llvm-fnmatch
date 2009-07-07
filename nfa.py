#!/usr/bin/env python

from characterset import CharacterSet, distinctCharacterSets
from fsm import State, StateMachine

class NFAState(State):
  pass


class NFA(StateMachine):
  @classmethod
  def fnmatch(klass, s):
    '''create an NFA state machine representing the fnmatch pattern @s'''
    nfa = NFA(NFAState())
    state = nfa.initial
    new_state = None
    chars = list(s)
    while True:
      if len(chars) == 0: break # end-of-string
      c = chars.pop(0)
      new_state = NFAState()
      nfa.states.append(new_state)
      if c == '?':
        # single-character wildcard
        state.add(CharacterSet.excluding(''), new_state)
      elif c == '*':
        # multi-character wildcard
        state.add(CharacterSet.excluding(''), new_state)
        new_state.add(CharacterSet.excluding(''), new_state)
      elif c == '\\':
        # treat the next character literally
        if len(chars) == 0:
          raise 'escape at end of string'
        c = chars.pop(0)
        state.add(CharacterSet.including(c), new_state)
      elif c == '[':
        # bracket expression
        try:
          c = chars.pop(0)
          if c == '!': # inverted
            inverted = True
            c = chars.pop(0)
          else:
            inverted = False
          charset = CharacterSet.including('')
          last_char = None
          while c != ']':
            if c == '-' and last_char:
              c = chars.pop(0)
              charset = charset.union(CharacterSet.range(last_char, c))
              last_char = None
              c = chars.pop(0)
              continue
            charset = charset.union(CharacterSet.including(c))
            last_char = c # save last character
            c = sio.read(1)
        except IndexError, e:
          raise 'unterminated bracket expression'
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

