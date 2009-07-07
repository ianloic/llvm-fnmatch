class CharacterSet:
  '''represents a set of characters, defined either by what is included in the
  set or by what is excluded from the set'''
  
  def __init__(self, inclusive, characters):
    '''call CharacterSet.including(), CharacterSet.excluding() or
    CharacterSet.range() instead of this'''
    self.inclusive = bool(inclusive)
    self.characters = frozenset(characters)

  @classmethod
  def including(klass, characters):
    '''construct a new CharacterSet including just the provided characters'''
    return klass(True, characters)

  @classmethod
  def excluding(klass, characters):
    '''construct a new CharacterSet containing all characters except the ones
    provided'''
    return klass(False, characters)

  @classmethod
  def range(klass, start, end):
    '''construct a new CharacterSet containing all characters in the range (inclusive)'''
    assert isinstance(start, basestring) and len(start) == 1
    assert isinstance(end, basestring) and len(end) == 1
    
    if end < start:
      start, end = end, start
    return klass(True, [chr(c) for c in range(ord(start), ord(end)+1)])

  def __contains__(self, c):
    '''is the supplied character in the set?'''
    assert isinstance(c, basestring) and len(c) == 1
    
    if self.inclusive:
      return c in self.characters
    else:
      return not (c in self.characters)

  def __str__(self):
    '''when displaying this set to the user, how should we present it?'''
    # sort the list of characters
    chars = list(self.characters)
    chars.sort()
    if self.inclusive:
      return `''.join(chars)`
    else:
      if not chars: return 'ANY'
      else: return 'NOT '+`''.join(chars)`
      
  def __eq__(self, other):
    assert isinstance(other, CharacterSet)
    return self.inclusive == other.inclusive and \
        self.characters == other.characters
  def __ne__(self, other):
    assert isinstance(other, CharacterSet)
    return not (self == other)
  def __hash__(self):
    return hash('%s %s' % (self.inclusive, ''.join(self.characters)))

  def union(self, other):
    '''set union'''
    assert isinstance(other, CharacterSet)
    
    if self.inclusive and other.inclusive:
      return CharacterSet.including(self.characters.union(other.characters))
    elif not self.inclusive and not other.inclusive:
      return CharacterSet.excluding(self.characters.intersection(other.characters))
    elif self.inclusive:
      return CharacterSet.excluding(other.characters-self.characters)
    else:
      return CharacterSet.excluding(self.characters-other.characters)

  def __sub__(self, other):
    '''set difference'''
    assert isinstance(other, CharacterSet)
    
    if self.inclusive and other.inclusive:
      return CharacterSet.including(self.characters-other.characters)
    elif not self.inclusive and not other.inclusive:
      return CharacterSet.including(other.characters-self.characters)
    elif self.inclusive and not other.inclusive:
      return CharacterSet.including(self.characters.intersection(other.characters))
    elif not self.inclusive and other.inclusive:
      return CharacterSet.excluding(self.characters.union(other.characters))
    else:
      raise 'wtf?'

  def intersection(self, other):
    '''set intersection'''
    assert isinstance(other, CharacterSet)
    
    return self - (self - other)

  def all(self):
    '''does the set match all characters?'''
    return not self.inclusive and not self.characters

  def empty(self):
    '''does the set match no characters?'''
    return self.inclusive and not self.characters

  def __repr__(self):
    return 'CharacterSet(%s, %s)' % (`self.inclusive`, `''.join(self.characters)`)

  def disjoint(self, other):
    '''does the set not intersect the other?'''
    assert isinstance(other, CharacterSet)
    return (self.intersection(other)).empty()
    
  def intersects(self, other):
    '''does the set intersect another?'''
    assert isinstance(other, CharacterSet)
    return not self.disjoint(other)


def __disjoin(charsets, charset):
  '''@charsets is a set of disjoint charsets, @charset is a charset.
  return a set of charsets with the same range as all inputs, but all disjoint'''
  # prove preconditions
  assert isinstance(charsets, (set, frozenset))
  for cs in charsets:
    assert isinstance(cs, CharacterSet)
  assert isinstance(charset, CharacterSet)

  if len(charsets) == 0: return frozenset([charset])

  result = frozenset([cs-charset for cs in charsets])
  result = result.union(frozenset([cs.intersection(charset) for cs in charsets]))
  charsets_union = reduce(lambda a,b:a.union(b), charsets)
  result = result.union(frozenset([charset-charsets_union]))
  return result



def distinctCharacterSets(orig_charsets):
  '''from the original set of charsets find a new set of charsets such that:
   * the union of the original charsets is equal to the union of the new 
     charsets
   * none of the new character sets intersect
   * each original charset is equal to the union of one or more new charsets
  '''
  
  # original_charsets contains the set of original charsets
  charsets = frozenset(orig_charsets)

  # find disjoint sets
  partition = reduce(__disjoin, charsets, frozenset())

  # filter the empty set out of the partition
  partition = frozenset([charset for charset in partition if not charset.empty()])

  # check that our result matches our contract
  # make sure that none of our character sets intersect
  union_out = CharacterSet.including('') # empty set
  for cs in partition:
    assert cs.disjoint(union_out)
    union_out = union_out.union(cs)
  # make sure that the union of result character sets == the union of the input character sets
  union_in = reduce(lambda a,b:a.union(b), orig_charsets)
  assert union_in == union_out

  return partition


def test_CharacterSet():
  # test .all() and .empty()
  assert CharacterSet.excluding('').all()
  assert CharacterSet.including('').empty()
 
  # test that character order is irrelevant
  assert CharacterSet.including('ab') == CharacterSet.including('ba')
  assert CharacterSet.excluding('ab') == CharacterSet.excluding('ba')

  # test equality and inequality with varying inclusivity
  assert CharacterSet.including('ab') == CharacterSet.including('ab')
  assert (CharacterSet.including('ab') != CharacterSet.including('ab')) == False
  assert CharacterSet.including('ab') != CharacterSet.excluding('ab')
  assert (CharacterSet.including('ab') == CharacterSet.excluding('ab')) == False

  # test union with inclusive character sets
  assert CharacterSet.including('ab').union(CharacterSet.including('bc')) == CharacterSet.including('abc')
  assert CharacterSet.including('ab').union(CharacterSet.including('cd')) == CharacterSet.including('abcd')

  # test union with inclusive and exclusive character sets
  assert CharacterSet.including('ab').union(CharacterSet.excluding('')) == CharacterSet(False,'')
  assert CharacterSet.including('ab').union(CharacterSet.excluding('cd')) == CharacterSet(False,'cd')
  assert CharacterSet.including('ab').union(CharacterSet.excluding('bc')) == CharacterSet(False,'c')
  assert CharacterSet.excluding('').union(CharacterSet.including('ab')) == CharacterSet(False,'')
  assert CharacterSet.excluding('cd').union(CharacterSet.including('ab')) == CharacterSet(False,'cd')
  assert CharacterSet.excluding('bc').union(CharacterSet.including('ab')) == CharacterSet(False,'c')

  # test union with exclusive character sets
  assert CharacterSet.excluding('ab').union(CharacterSet.excluding('bc')) == CharacterSet.excluding('b')
  assert CharacterSet.excluding('ab').union(CharacterSet.excluding('cd')) == CharacterSet.excluding('')

  # test difference with inclusive character sets
  assert CharacterSet.including('ab') - CharacterSet.including('bc') == CharacterSet.including('a')
  assert CharacterSet.including('ab') - CharacterSet.including('cd') == CharacterSet.including('ab')

  # test difference with inclusive and exclusive character sets
  assert CharacterSet.including('ab') - CharacterSet.excluding('') == CharacterSet(True,'')
  assert CharacterSet.including('ab') - CharacterSet.excluding('cd') == CharacterSet(True,'')
  assert CharacterSet.including('ab') - CharacterSet.excluding('bc') == CharacterSet(True,'b')
  assert CharacterSet.excluding('') - CharacterSet.including('ab') == CharacterSet(False,'ab')
  assert CharacterSet.excluding('cd') - CharacterSet.including('ab') == CharacterSet(False,'abcd')
  assert CharacterSet.excluding('bc') - CharacterSet.including('ab') == CharacterSet(False,'abc')
  assert CharacterSet.including('') - CharacterSet.excluding('') == CharacterSet.including('')
  assert CharacterSet.excluding('') - CharacterSet.including('') == CharacterSet.excluding('')

  # test difference with exclusive character sets
  assert CharacterSet.excluding('ab') - CharacterSet.excluding('bc') == CharacterSet.including('c')
  assert CharacterSet.excluding('ab') - CharacterSet.excluding('cd') == CharacterSet.including('cd')

  # test intersection with inclusive character sets
  assert CharacterSet.including('ab').intersection(CharacterSet.including('bc')) == CharacterSet.including('b')
  assert CharacterSet.including('ab').intersection(CharacterSet.including('cd')) == CharacterSet.including('')

  # test intersection with inclusive and exclusive character sets
  assert CharacterSet.including('ab').intersection(CharacterSet.excluding('')) == CharacterSet.including('ab')
  assert CharacterSet.including('ab').intersection(CharacterSet.excluding('cd')) == CharacterSet.including('ab')
  assert CharacterSet.including('ab').intersection(CharacterSet.excluding('bc')) == CharacterSet.including('a')
  assert CharacterSet.excluding('').intersection(CharacterSet.including('ab')) == CharacterSet.including('ab')
  assert CharacterSet.excluding('cd').intersection(CharacterSet.including('ab')) == CharacterSet.including('ab')
  assert CharacterSet.excluding('bc').intersection(CharacterSet.including('ab')) == CharacterSet.including('a')
  assert CharacterSet.excluding('').intersection(CharacterSet.including('')) == CharacterSet.including('')
  assert CharacterSet.including('').intersection(CharacterSet.excluding('')) == CharacterSet.including('')

  # test all this disjoint set crap
  assert distinctCharacterSets([CharacterSet.including('')]) == set()
  assert distinctCharacterSets([CharacterSet.excluding('')]) == set([CharacterSet.excluding('')])
  assert distinctCharacterSets([CharacterSet.excluding('abc')]) == set([CharacterSet.excluding('abc')])
  assert distinctCharacterSets([CharacterSet.including('abc')]) == set([CharacterSet.including('abc')])

  assert distinctCharacterSets([CharacterSet.including(''), CharacterSet.excluding('')]) == \
      set([CharacterSet.excluding('')])
  assert distinctCharacterSets([CharacterSet.including('abc'), CharacterSet.excluding('')]) == \
      set([CharacterSet.excluding('abc'), CharacterSet.including('abc')])
  assert distinctCharacterSets([CharacterSet.including('abc'), CharacterSet.excluding('abc')]) == \
      set([CharacterSet.excluding('abc'), CharacterSet.including('abc')])
  assert distinctCharacterSets([CharacterSet.including('abc'), CharacterSet.excluding('cde')]) == \
      set([CharacterSet.including('ab'), CharacterSet.including('c'), CharacterSet(False,'abcde')])
  assert distinctCharacterSets([CharacterSet.including('abc'), CharacterSet.excluding('def')]) == \
      set([CharacterSet.including('abc'), CharacterSet(False,'abcdef')])

  assert distinctCharacterSets([CharacterSet.including('abc'), CharacterSet.including('abc')]) == \
      set([CharacterSet.including('abc')])
  assert distinctCharacterSets([CharacterSet.including('abc'), CharacterSet.including('cde')]) == \
      set([CharacterSet.including('ab'), CharacterSet.including('c'), CharacterSet.including('de'),])
  assert distinctCharacterSets([CharacterSet.including('abc'), CharacterSet.including('def')]) == \
      set([CharacterSet.including('abc'), CharacterSet.including('def')])

  assert distinctCharacterSets([CharacterSet.excluding('abc'), CharacterSet.excluding('abc')]) == \
      set([CharacterSet.excluding('abc')])
  assert distinctCharacterSets([CharacterSet.excluding('abc'), CharacterSet.excluding('def')]) == \
      set([CharacterSet.including('abc'), CharacterSet.including('def'), CharacterSet.excluding('abcdef')])
  assert distinctCharacterSets([CharacterSet.excluding('abc'), CharacterSet.excluding('cde')]) == \
      set([CharacterSet.including('ab'), CharacterSet.including('de'), CharacterSet.excluding('abcde')])

  assert distinctCharacterSets([CharacterSet.including('a'), CharacterSet.including('b'), CharacterSet.including('c')]) == \
      set([CharacterSet.including('a'), CharacterSet.including('b'), CharacterSet.including('c')])
  assert distinctCharacterSets([CharacterSet.including('abc'), CharacterSet.including('b'), CharacterSet.including('c')]) == \
      set([CharacterSet.including('a'), CharacterSet.including('b'), CharacterSet.including('c')])
  assert distinctCharacterSets([CharacterSet.including('abc'), CharacterSet.including('b'), CharacterSet.including('cde')]) == \
      set([CharacterSet.including('a'), CharacterSet.including('b'), CharacterSet.including('c'), CharacterSet.including('de')])


