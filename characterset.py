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
    if self.inclusive and other.inclusive:
      return CharacterSet(True, self.characters.union(other.characters))
    elif not self.inclusive and not other.inclusive:
      return CharacterSet(False, self.characters.intersection(other.characters))
    elif self.inclusive:
      return CharacterSet(False, other.characters-self.characters)
    else:
      return CharacterSet(False, self.characters-other.characters)

  def __sub__(self, other):
    if self.inclusive and other.inclusive:
      return CharacterSet(True, self.characters-other.characters)
    elif not self.inclusive and not other.inclusive:
      return CharacterSet(True, other.characters-self.characters)
    elif self.inclusive and not other.inclusive:
      return CharacterSet(True, self.characters.intersection(other.characters))
    elif not self.inclusive and other.inclusive:
      return CharacterSet(False, self.characters.union(other.characters))
    else:
      raise 'wtf?'

  def intersection(self, other):
    return self - (self - other)

  def all(self):
    '''matches all characters'''
    return not self.inclusive and not self.characters

  def empty(self):
    '''matches no characters'''
    return self.inclusive and not self.characters

  def __repr__(self):
    return 'CharacterSet(%s, %s)' % (`self.inclusive`, `''.join(self.characters)`)

#  def distinct(self, other):
#    '''return pair of tuples of sets. none of the returned sets intersect,
#    the union of each pair is equal to self or other, respectively'''
#    return ((self-other, self.intersection(other)), (other-self, self.intersection(other)))
  def distinct(self, other):
    return (self-other, other-self, self.intersection(other))


def distinctCharacterSets(orig_charsets):
  '''for a set of charsets, find a set of disjoint charsets
  whose union is equal to the union of the original charsets'''
  # original_charsets contains the set of original charsets
  charsets = set(orig_charsets)
  #print '\ndistinctCharacterSets(%s)' % `charsets`
  # partition will hold the partition of the union of those sets
  #print `charsets`
  partition = set((charsets.pop(),))
  #print `charsets`
  for o in charsets:
    new_partition = set()
    for p in partition:
      new_partition = new_partition.union(set((p-o, o-p, o.intersection(p))))
    partition = new_partition

  # filter the empty set out of the partition
  partition = set([charset for charset in partition if not charset.empty()])

  #print 'partitioned %s into %s' % (`orig_charsets`, `partition`)

  # check to make sure we didn't screw up
  if partition and orig_charsets:
    assert reduce(lambda x,y:x.union(y), partition) == \
        reduce(lambda x,y:x.union(y), orig_charsets)

  return partition


def test_CharacterSet():
  # test .all() and .empty()
  assert CharacterSet(False, '').all()
  assert CharacterSet(True, '').empty()
 
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
  assert CharacterSet(False, 'ab').union(CharacterSet(False, 'bc')) == CharacterSet(False, 'b')
  assert CharacterSet(False, 'ab').union(CharacterSet(False, 'cd')) == CharacterSet(False, '')

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

  # test all this disjoint set crap
  assert distinctCharacterSets([CharacterSet(True, '')]) == set()
  assert distinctCharacterSets([CharacterSet(False, '')]) == set([CharacterSet(False, '')])
  assert distinctCharacterSets([CharacterSet(False, 'abc')]) == set([CharacterSet(False, 'abc')])
  assert distinctCharacterSets([CharacterSet(True, 'abc')]) == set([CharacterSet(True, 'abc')])

  assert distinctCharacterSets([CharacterSet(True, ''), CharacterSet(False, '')]) == \
      set([CharacterSet(False, '')])
  assert distinctCharacterSets([CharacterSet(True, 'abc'), CharacterSet(False, '')]) == \
      set([CharacterSet(False, 'abc'), CharacterSet(True, 'abc')])
  assert distinctCharacterSets([CharacterSet(True, 'abc'), CharacterSet(False, 'abc')]) == \
      set([CharacterSet(False, 'abc'), CharacterSet(True, 'abc')])
  assert distinctCharacterSets([CharacterSet(True, 'abc'), CharacterSet(False, 'cde')]) == \
      set([CharacterSet(True, 'ab'), CharacterSet(True, 'c'), CharacterSet(False,'abcde')])
  assert distinctCharacterSets([CharacterSet(True, 'abc'), CharacterSet(False, 'def')]) == \
      set([CharacterSet(True, 'abc'), CharacterSet(False,'abcdef')])

  assert distinctCharacterSets([CharacterSet(True, 'abc'), CharacterSet(True, 'abc')]) == \
      set([CharacterSet(True, 'abc')])
  assert distinctCharacterSets([CharacterSet(True, 'abc'), CharacterSet(True, 'cde')]) == \
      set([CharacterSet(True, 'ab'), CharacterSet(True, 'c'), CharacterSet(True, 'de'),])
  assert distinctCharacterSets([CharacterSet(True, 'abc'), CharacterSet(True, 'def')]) == \
      set([CharacterSet(True, 'abc'), CharacterSet(True, 'def')])

  assert distinctCharacterSets([CharacterSet(False, 'abc'), CharacterSet(False, 'abc')]) == \
      set([CharacterSet(False, 'abc')])
  assert distinctCharacterSets([CharacterSet(False, 'abc'), CharacterSet(False, 'def')]) == \
      set([CharacterSet(True, 'abc'), CharacterSet(True, 'def'), CharacterSet(False, 'abcdef')])
  assert distinctCharacterSets([CharacterSet(False, 'abc'), CharacterSet(False, 'cde')]) == \
      set([CharacterSet(True, 'ab'), CharacterSet(True, 'de'), CharacterSet(False, 'abcde')])

  assert distinctCharacterSets([CharacterSet(True, 'a'), CharacterSet(True, 'b'), CharacterSet(True, 'c')]) == \
      set([CharacterSet(True, 'a'), CharacterSet(True, 'b'), CharacterSet(True, 'c')])
  assert distinctCharacterSets([CharacterSet(True, 'abc'), CharacterSet(True, 'b'), CharacterSet(True, 'c')]) == \
      set([CharacterSet(True, 'a'), CharacterSet(True, 'b'), CharacterSet(True, 'c')])
  assert distinctCharacterSets([CharacterSet(True, 'abc'), CharacterSet(True, 'b'), CharacterSet(True, 'cde')]) == \
      set([CharacterSet(True, 'a'), CharacterSet(True, 'b'), CharacterSet(True, 'c'), CharacterSet(True, 'de')])


