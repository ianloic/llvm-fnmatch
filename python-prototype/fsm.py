# base classes for finite state machine

class State:
  __id = 0
  def __init__(self, name, match=False):
    self.children = []
    self.match = match
    self.name = name
    self.id = '%s_%d' % (self.__class__.__name__, State.__id)
    State.__id = State.__id + 1

  def add(self, charset, state):
    self.children.append((charset, state))

  def dot(self, dot):
    if self.match:
      dot.node(self.id, self.name, peripheries=2)
    else:
      dot.node(self.id, self.name)
    for charset, child in self.children:
      dot.arc(self.id, child.id, charset.label())

  def __call__(self, c):
    return [node for charset, node in self.children if c in charset]


class StateMachine:
  def __init__(self, initial, states=[]):
    self.initial = initial
    self.states = states
    if self.initial not in self.states:
      self.states.append(self.initial)

  def dot(self, dot): 
    for state in self.states:
      state.dot(dot)
