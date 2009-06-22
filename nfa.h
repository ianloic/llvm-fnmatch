#ifndef __nfa_h__
#define __nfa_h__

#include <string>
#include <algorithm>
#include <map>

#include "characterset.h"

#include "fsm.h"

class NFAState : public FSMState, 
                 public std::multimap<CharacterSet, FSMState*> {
  public:
    virtual void dot() {
      printf("\tDFA%d [label=\"%d\"]\n", Id(), Id());
      for (iterator iter = begin(); iter != end(); iter++) {
        printf("\tDFA%d -> DFA%d [label=\"%s\"]\n", Id(), 
            iter->second->Id(), iter->first.Label().c_str());
      }
    }

    virtual void add(const CharacterSet& aCharacterSet, FSMState* aState) {
      insert(FSMState::pair(aCharacterSet, aState));
    }
};


class NFA : public FSM<NFAState> {
  public:
    static NFA* fnmatch(std::string pattern) {
      NFA *nfa = new NFA();

      nfa->mInitial = nfa->addState();
      
      NFAState* state = nfa->mInitial;
      state->SetName(std::string("INITIAL"));

      NFAState* new_state = NULL;

      size_t i=0;
      while(i < pattern.size()) {
        new_state = nfa->addState();
        new_state->SetName(std::string(pattern, i, 1));

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
};


#endif // __nfa_h__
