#ifndef __dfa_h__
#define __dfa_h__

#include <string>
#include <algorithm>
#include <map>

#include "characterset.h"
#include "fsm.h"
#include "nfa.h"

class DFAState : public FSMState, 
                 public std::map<CharacterSet, FSMState*> {
  public:
    virtual void add(const CharacterSet& aCharacterSet, FSMState* aState) {
      insert(FSMState::pair(aCharacterSet, aState));
    }
};

class DFA : FSM<DFAState> {
  public:

    static DFA* FromNFA(const NFA& aNFA) {
      return NULL;
    }

};



#endif // __dfa_h__

