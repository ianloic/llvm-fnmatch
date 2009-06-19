#ifndef __dfa_h__
#define __dfa_h__

#include <string>
#include <algorithm>
#include <map>
#include <vector>

#include "characterset.h"
#include "nfa.h"

class DFA {
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
        typedef std::map<CharacterSet, State*> map;
        State::map mChildren;
        int mId;
        // FIXME: implement private operator= and copy constructor?
      protected:
        State(NFA::State* aNFAState) 
          : mMatch(false), mName(aName) { 
          static int sId = 0;
          mId = sId++;
          // if any of the NFA states associated with this DFA state then this
          // state represents a match.
          for(std::vector<NFA::State*>::iterator iter = 
              aNFAState->mStates.begin(); 
              iter != aNFAState->mStates.end(); iter++) {
            if (iter->GetMatch()) {
              mMatch = true;
              break;
            }
          }
          
        }
        friend class DFA;
    };

    State* addState(const std::string& aName, bool aMatch) {
      State* state = new State(aName, aMatch);
      mStates.push_back(state);
      return state;
    }

    ~DFA() {
      // delete all of the states associated with this machine
      for(std::vector<State*>::iterator i = mStates.begin();
          i < mStates.end(); i++) {
        delete *i;
      }
    }


    static DFA* FromNFA(NFA* aNFA) {
    }


    void dot() {
      printf("digraph foo {\n\trankdir=LR\n");

      for (size_t i=0; i<mStates.size(); i++) {
        State* state = mStates[i];
        printf("\tDFA%d [label=\"%d\"]\n", state->Id(), state->Id());
        for (State::map::iterator iter = state->mChildren.begin(); 
            iter != state->mChildren.end(); iter++) {
          printf("\tDFA%d -> DFA%d [label=\"%s\"]\n", state->Id(), 
              (*iter).second->Id(), (*iter).first.Label().c_str());
        }
      }

      printf("}\n");
    };

  private:
    State* mInitial;
    std::vector<State*> mStates;
};


typedef map<CharacterSet,>


#endif // __dfa_h__

