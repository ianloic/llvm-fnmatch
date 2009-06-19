#ifndef __nfa_h__
#define __nfa_h__

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
        friend class DFA;
    };

    typedef std::set<State*> States;

    State* addState(const std::string& aName, bool aMatch) {
      State* state = new State(aName, aMatch);
      mStates.insert(state);
      return state;
    }

    ~NFA() {
      // delete all of the states associated with this machine
      for(States::iterator i = mStates.begin(); i != mStates.end(); i++) {
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

      for(States::iterator i = mStates.begin(); i != mStates.end(); i++) {
        State* state = *i;
        printf("\tNFA%d [label=\"%d\"]\n", state->Id(), state->Id());
        for (State::map::iterator iter = state->mChildren.begin(); 
            iter != state->mChildren.end(); iter++) {
          printf("\tNFA%d -> NFA%d [label=\"%s\"]\n", state->Id(), 
              (*iter).second->Id(), (*iter).first.Label().c_str());
        }
      }

      printf("}\n");
    };

    typedef std::map<CharacterSet,States> Arcs;
    static Arcs DistinctArcs(const Arcs& aArcs) {
      if (aArcs.size() < 2) {
        return aArcs;
      }

      // find the distinct character sets we have
      CharacterSets partition;
      for (Arcs::const_iterator iter = aArcs.begin();
          iter != aArcs.end(); iter++) {
        partition.insert((*iter).first);
      }
      partition = partition.Distinct();

      // reassociate those disjoint character sets with the right states
      Arcs arcs;
      for (CharacterSets::const_iterator iter = partition.begin();
          iter != partition.end(); iter++) {
        States states;
        for (Arcs::const_iterator arcs_iter = aArcs.begin();
            arcs_iter != aArcs.end(); arcs_iter++) {
          if ((*arcs_iter).first.Intersects(*iter)) {
            // copy the states from this arc onto the arc
            const States& other_states = (*arcs_iter).second;
            std::copy(other_states.begin(), other_states.end(),
                std::inserter(states, states.begin()));
          }
        }
        if (!states.empty()) {
          arcs.insert(Arcs::value_type(*iter, states));
        }
      }

      // if we have multiple charsets associated with the same set of states
      // then lets merge those arcs
      // to do this we need to reverse the arcs map...
      typedef std::map<States,CharacterSet> AntiArcs;
      AntiArcs charset_by_states;
      for (Arcs::const_iterator iter=arcs.begin(); iter != arcs.end(); iter++) {
        AntiArcs::iterator pos = charset_by_states.find((*iter).second);
        if (pos == charset_by_states.end()) {
          // haven't seen these states yet...
          charset_by_states.insert(
              AntiArcs::value_type((*iter).second, (*iter).first));
        } else {
          // seen these states before, let's update the CharacterSet to
          // include the one we just found
          charset_by_states.insert(pos,
              AntiArcs::value_type(pos->first, 
                (*pos).second.Union((*iter).first)));
        }
      }

      // put the arcs back together
      arcs.clear();
      for (AntiArcs::const_iterator iter = charset_by_states.begin();
          iter != charset_by_states.end(); iter++) {
        arcs.insert(Arcs::value_type());
      }

      return arcs;
    }

  private:
    State* mInitial;
    States mStates;
};


#endif // __nfa_h__
