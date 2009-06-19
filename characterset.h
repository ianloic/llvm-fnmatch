#ifndef __charset_h__
#define __charset_h__

#include <string>
#include <sstream>
#include <set>
#include <algorithm>

  class CharacterSets;

  class CharacterSet {
    // CharacterSet is an immutable class representing a set of chars.
    // It can be inclusive (listing all the included chars) or exclusive
    // listing all of the chars not considered to be part of the set.
    // It does not follow the patterns of std::set.
    public:
      // construct a CharacterSet containing all chars in @aChars
      static CharacterSet Including(const std::string& aChars) {
        return CharacterSet(true, aChars);
      }

      // construct a CharacterSet containing just the character @aChar
      static CharacterSet Including(char aChar) {
        std::set<char> chars;
        chars.insert(aChar);
        return CharacterSet::Including(chars);
      }

      // construct a CharacterSet containing all chars except for those
      // in @aChars
      static CharacterSet Excluding(const std::string& aChars) {
        return CharacterSet(false, aChars);
      }

      // construct a CharacterSet containing all chars in the range
      // @aState to @aEnd, inclusive
      static CharacterSet Range(char aStart, char aEnd) {
        if (aEnd < aStart) {
          char tmp = aEnd;
          aEnd = aStart;
          aStart = tmp;
        }
        std::set<char> chars;
        for (char i=aStart; i<=aEnd; i++) {
          chars.insert(i);
        }
        return CharacterSet::Including(chars);
      }

      CharacterSet() 
        : mInclusive(true), mChars() { }

      // does this CharacterSet contain the character @aCharacter?
      bool Contains(char aCharacter) const {
        return mChars.find(aCharacter) != mChars.end();
      }

      // FIXME: implement label() ?

      // equality operators
      bool operator==(const CharacterSet& aOther) const {
        return mInclusive == aOther.mInclusive && 
          mChars == aOther.mChars;
      }
      bool operator!=(const CharacterSet& aOther) const {
        return mInclusive != aOther.mInclusive ||
          mChars != aOther.mChars;
      }

      // fixme implement hash? is it required? how do we do that?
      
      const CharacterSet Union(const CharacterSet& aOther) const {
        if (mInclusive && aOther.mInclusive) {
          std::set<char> chars;
          std::set_union(mChars.begin(), mChars.end(),
              aOther.mChars.begin(), aOther.mChars.end(),
              std::inserter(chars, chars.begin()));
          return CharacterSet::Including(chars);
        } else if (!mInclusive && !aOther.mInclusive) {
          std::set<char> chars;
          set_intersection(mChars.begin(), mChars.end(),
              aOther.mChars.begin(), aOther.mChars.end(),
              std::inserter(chars, chars.begin()));
          return CharacterSet::Excluding(chars);
        } else if (mInclusive) {
          std::set<char> chars;
          set_difference(aOther.mChars.begin(), aOther.mChars.end(),
              mChars.begin(), mChars.end(), 
              std::inserter(chars, chars.begin()));
          return CharacterSet::Excluding(chars);
        } else {
          std::set<char> chars;
          set_difference(mChars.begin(), mChars.end(),
              aOther.mChars.begin(), aOther.mChars.end(),
              std::inserter(chars, chars.begin()));
          return CharacterSet::Excluding(chars);
        }
      }

      const CharacterSet Difference(const CharacterSet& aOther) const {
        if (mInclusive && aOther.mInclusive) {
          std::set<char> chars;
          std::set_difference(mChars.begin(), mChars.end(),
              aOther.mChars.begin(), aOther.mChars.end(),
              std::inserter(chars, chars.begin()));
          return CharacterSet::Including(chars);
        } else if (!mInclusive && !aOther.mInclusive) {
          std::set<char> chars;
          set_difference(aOther.mChars.begin(), aOther.mChars.end(), 
              mChars.begin(), mChars.end(),
              std::inserter(chars, chars.begin()));
          return CharacterSet::Including(chars);
        } else if (mInclusive) {
          std::set<char> chars;
          std::set_intersection(mChars.begin(), mChars.end(),
              aOther.mChars.begin(), aOther.mChars.end(),
              std::inserter(chars, chars.begin()));
          return CharacterSet::Including(chars);
        } else {
          std::set<char> chars;
          set_union(mChars.begin(), mChars.end(),
              aOther.mChars.begin(), aOther.mChars.end(),
              std::inserter(chars, chars.begin()));
          return CharacterSet::Excluding(chars);
        }
      }

      const CharacterSet Intersection(const CharacterSet& aOther) const {
        return Difference(Difference(aOther));
      }

      bool All() const {
        return !mInclusive && mChars.empty();
      }

      bool Empty() const {
        return mInclusive && mChars.empty();
      }

      bool Disjoint(const CharacterSet& aOther) const {
        return Intersection(aOther).Empty();
      }
      bool Intersects(const CharacterSet& aOther) const {
        return !Disjoint(aOther);
      }

      // for std::less
      bool operator<(const CharacterSet& aOther) const {
        if (mInclusive != aOther.mInclusive) {
          return mInclusive < aOther.mInclusive;
        }
        return mChars < aOther.mChars;
      }

      // a textual representation...
      std::string Label() const {
        if (mChars.empty()) {
          if (mInclusive) {
            return std::string("NONE");
          } else {
            return std::string("ALL");
          }
        } else {
          std::stringstream ss;
          if (!mInclusive) {
            ss << "NOT: ";
          }
          ss << "\\\"";

          for (std::set<char>::iterator iter = mChars.begin();
              iter != mChars.end(); iter++) {
            ss << *iter;
          }
          ss << "\\\"";
          return ss.str();
        }
      }


    private:
      static CharacterSet Including(const std::set<char>& aChars) {
        return CharacterSet(true, aChars);
      }

      static CharacterSet Excluding(const std::set<char>& aChars) {
        return CharacterSet(false, aChars);
      }

      CharacterSet(bool aInclusive, const std::set<char>& aChars)
          : mInclusive(aInclusive), mChars(aChars) { 
      }

      CharacterSet(bool aInclusive, const std::string& aChars)
          : mInclusive(aInclusive) { 
        // FIXME: is there a more efficient way to do this?
        for (size_t i=0; i<aChars.size(); i++) {
          mChars.insert(aChars[i]);
        }
      }


      bool mInclusive;
      std::set<char> mChars;
  };

  // CharacterSets is a std::set of CharacterSet, yeah sets of sets, 
  // it's confusing
  class CharacterSets : public std::set<CharacterSet> {
    public:
      // the important function. return a set of CharacterSets with the same
      // range as all of the inputs, but disjoint
      CharacterSets Disjoin(const CharacterSet& aCharSet) const {
        // if @this is empty, return a new set containing just @aCharSet
        if (empty()) {
          CharacterSets css;
          css.insert(aCharSet);
          return css;
        }

        CharacterSets result;

        // for each charset X in @this, we want X-@aCharSet and the
        // intersection of X and @aCharSet, 
        // also
        // for U is the union of all of the sets in @this, we want
        // @aCharSet-U
        CharacterSet U;
        for (const_iterator X = begin(); X != end(); X++) {
          // X - aCharset
          result.insert((*X).Difference(aCharSet));
          // X ∩ aCharset
          result.insert((*X).Intersection(aCharSet));
          // U ∪ X
          U = U.Union((*X));
        }
        // aCharset - U
        result.insert(aCharSet.Difference(U));

        return result;
      }
      CharacterSets Distinct() const {
        // identify a set of disjoint CharacterSet such that for each input
        // CharacterSet X there is a set of output CharacterSet X' whose
        // union is equal to X. we call this our partition
        CharacterSets partition;
        for (const_iterator iter = begin(); iter != end(); iter++) {
          partition = partition.Disjoin(*iter);
        }

        // eliminate any empty CharacterSet from the partition
        CharacterSets nonempty;
        for (const_iterator iter = partition.begin();
            iter != partition.end(); iter++) {
          if (!(*iter).Empty()) {
            nonempty.insert((*iter));
          }
        }

        return nonempty;
      }
  };

#endif // __charset_h__
