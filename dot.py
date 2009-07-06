
class Dot:
  def __init__(self, name, label=None):
    self.name = name
    self.label = label
    self.nodes = []
    self.arcs = []

  def add(self, fsm):
    '''add a finite state machine to this diagram'''
    for state in fsm:
      if state.match:
        self.node(state.id, state, peripheries=2)
      else:
        self.node(state.id, state)
      for charset, child in state:
        self.arc(state.id, child.id, charset)

  def node(self, name, label=None, peripheries=1):
    self.nodes.append((name, label, peripheries))

  def arc(self, node1, node2, label=None):
    self.arcs.append((node1, node2, label))

  def __str__(self):
    s = 'digraph %s {\n\trankdir=LR\n' % self.name
    
    if self.label != None:
      s = s + ('\tlabel="%s"\n' % self.label)

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
    from os import popen, system
    from tempfile import mktemp
    filename = '%s.png' % mktemp()
    pipe = popen('dot -Tpng -o %s' % filename, 'w')
    pipe.write(str(self))
    pipe.close()
    system('gnome-open %s' % filename)


if __name__ == '__main__':
  from optparse import OptionParser
  op = OptionParser()
  op.add_option('--nfa', dest='nfa', help='graph the NFA form', 
      action='store_true', default=False)
  op.add_option('--dfa', dest='dfa', help='graph the DFA form', 
      action='store_true', default=False)
  op.add_option('--show', dest='show', help='show the graph on the screen',
      action='store_true', default=False)
      
  (options, args) = op.parse_args()
  if len(args) < 1:
    print 'You must supply at least one fnmatch expression.'
  elif not options.nfa and not options.dfa:
    print 'You must request at least one of --nfa or --dfa'
  else:
    from nfa import NFA
    from dfa import DFA
    for pattern in args:
      dot = Dot('temp', pattern)
      nfa = NFA.fnmatch(pattern)
      if options.dfa:
        dfa = DFA(nfa)
        dot.add(dfa)
      if options.nfa:
        dot.add(nfa)
      if options.show:
        dot.show()
      else:
        print str(dot)
