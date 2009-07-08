# base classes for finite state machines

class State:
  '''State base class'''
  __id = 1
  def __init__(self, name=None, match=False, description=None):
    # a list of tuples (characterset, next_state) representing state transitions
    self.children = []
    # a boolean, is this state terminal
    self.match = match
    # an internal identifier, used internally graph generation
    self.id = '%s_%d' % (self.__class__.__name__, State.__id)
    # a human-readable name
    self.name = name
    # a human readable description
    self.description = description
    # use the id as the name if none is supplied
    if self.name == None: 
      self.name = str(State.__id)
    # give each state a unique id
    State.__id = State.__id + 1


  def __str__(self):
    '''a descriptive label that's used for debugging and visualization'''
    # create a label out of name and/or description
    if self.name and self.description:
      return '%s: %s' % (self.name, self.description)
    elif self.name:
      return self.name
    elif self.description:
      return self.description
    else:
      return ''

  def add(self, charset, state):
    '''add a child state'''
    self.children.append((charset, state))

  def __call__(self, c):
    '''return the set of states that are reached when @c is processed'''
    return frozenset((node for charset, node in self.children if c in charset))

  def __repr__(self):
    return '%s(%s)' % (self.__class__.__name__, self.name)

  def __iter__(self):
    '''return an iterator for the outbound arcs'''
    return iter(self.children)


class StateMachine:
  '''State machine base class'''
  def __init__(self, initial, states=[]):
    self.initial = initial
    self.states = states
    if self.initial not in self.states:
      self.states.append(self.initial)

  def __call__(self, s):
    states = set([self.initial])
    for c in s:
      if len(states) == 0:
        return False
      new_states = set()
      for state in states:
        new_states = new_states.union(state(c))
      states = new_states
    return len([s for s in states if s.match]) > 0

  def __iter__(self):
    '''return an iterator for all of the states in the machine'''
    return iter(self.states)


