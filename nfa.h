#ifndef __statemachine_h__
#define __statemachine_h__

#include <string>
#include <algorithm>
#include <map>
#include <vector>

#include "characterset.h"

class NFA {
  public:
    class State {
      public:
        void add(const CharacterSet& aCharacterSet, State* aState) {
          mChildren.insert(State::pair(aCharacterSet, aState));
        }
        bool operator<(const State* other) const {
          return this < other;
        }
        void SetMatch(bool aMatch) { mMatch = aMatch; }
        bool GetMatch() const { return mMatch; }
        int Id() { return mId; }
      private:
        bool mMatch;
        std::string mName;
        typedef std::pair<CharacterSet, State*> pair;
        typedef std::multimap<CharacterSet, State*> map;
        State::map mChildren;
        int mId;
        // FIXME: implement private operator= and copy constructor?
      protected:
        State(const std::string& aName, bool aMatch) 
          : mMatch(aMatch), mName(aName) { 
          static int sId = 0;
          mId = sId++;
        }
        friend class NFA;
    };

    State* addState(const std::string& aName, bool aMatch) {
      State* state = new State(aName, aMatch);
      mStates.push_back(state);
      return state;
    }

    ~NFA() {
      // delete all of the states associated with this machine
      for(std::vector<State*>::iterator i = mStates.begin();
          i < mStates.end(); i++) {
        delete *i;
      }
    }




    static NFA* fnmatch(std::string pattern) {
      NFA *nfa = new NFA();

      nfa->mInitial = nfa->addState(std::string("INITIAL"), false);
      
      State* state = nfa->mInitial;
      State* new_state = NULL;

      size_t i=0;
      while(i < pattern.size()) {
        new_state = nfa->addState(std::string(pattern, i, 1), false);

        if (pattern[i] == '?') {
          state->add(CharacterSet::Excluding(std::string()), new_state);
        } else if (pattern[i] == '*') {
          state->add(CharacterSet::Excluding(std::string()), new_state);
          new_state->add(CharacterSet::Excluding(std::string()), new_state);
        } else if (pattern[i] == '[') {
          i++;
          assert(i < pattern.size());

          bool inverted = false;
          if (pattern[i] == '!') {
            inverted = true;
            i++;
            assert(i < pattern.size());
          }

          CharacterSet charset;
          charset = CharacterSet();
          char last_char = '\0';

          while (pattern[i] != ']') {
            assert(i < pattern.size());

            if (pattern[i] == '-' && last_char) {
              i++;
              charset = charset.Union(
                  CharacterSet::Range(last_char, pattern[i]));
              last_char = '\0';
              i++;
              continue;
            }
            charset = charset.Union(CharacterSet::Including(pattern[i]));
            last_char = pattern[i];
            i++;
          }

          if (inverted) {
            state->add(CharacterSet::Excluding(std::string()).Difference(charset),
                new_state);
          } else {
            state->add(charset, new_state);
          }
        } else {
          state->add(CharacterSet::Including(pattern[i]), new_state);
        }
        state = new_state;
        i++;
      }
      state->SetMatch(true);
      return nfa;
    }

    void dot() {
      printf("digraph foo {\n\trankdir=LR\n");

      for (size_t i=0; i<mStates.size(); i++) {
        State* state = mStates[i];
        printf("\tNFA%d [label=\"%d\"]\n", state->Id(), state->Id());
        for (State::map::iterator iter = state->mChildren.begin(); 
            iter != state->mChildren.end(); iter++) {
          printf("\tNFA%d -> NFA%d [label=\"%s\"]\n", state->Id(), 
              (*iter).second->Id(), (*iter).first.Label().c_str());
        }
      }

      printf("}\n");
    };

  private:
    State* mInitial;
    std::vector<State*> mStates;
};


#endif // __statemachine_h__
