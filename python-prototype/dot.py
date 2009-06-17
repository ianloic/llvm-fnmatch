
class Dot:
  def __init__(self, name):
    self.name = name
    self.nodes = []
    self.arcs = []

  def node(self, name, label=None, peripheries=1):
    self.nodes.append((name, label, peripheries))

  def arc(self, node1, node2, label=None):
    self.arcs.append((node1, node2, label))

  def __str__(self):
    s = 'digraph %s {\n\trankdir=LR\n' % self.name

    for name, label, peripheries in self.nodes:
      if label:
        s = s + ('\t%s [label="%s",peripheries=%d]\n' % (name, label, peripheries))
      else:
        s = s + ('\t%s [peripheries=%d]\n' % (name, peripheries))

    for node1, node2, label in self.arcs:
      if label:
        s = s + ('\t%s -> %s [label="%s"]\n' % (node1, node2, label))
      else:
        s = s + ('\t%s -> %s\n' % (node1, node2))

    return s + '}\n'


  def show(self):
    from os import popen
    pipe = popen('dot -Txlib', 'w')
    pipe.write(str(self))
    pipe.close()

