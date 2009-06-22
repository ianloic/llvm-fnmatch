#ifndef __fsm_h__
#define __fsm_h__

#include <vector>

// Finite State Machine, or Flying Spaghetti Monster

class FSMState {
  public:
    typedef std::pair<CharacterSet, FSMState*> pair;
    FSMState() {
      static int sId = 0;
      mId = sId++;
    }
    bool operator<(const FSMState* other) const {
      return this < other;
    }
    void SetMatch(bool aMatch) { mMatch = aMatch; }
    bool GetMatch() const { return mMatch; }
    void SetName(const std::string& aName) { mName = aName; }
    std::string GetName() const { return mName; }
    int Id() { return mId; }
    virtual void dot() = 0;
    virtual void add(const CharacterSet& aCharacterSet, FSMState* aState) = 0;
    // FIXME: implement private operator= and copy constructor?
  protected:
    bool mMatch;
    std::string mName;
    int mId;
};

template<class StateClass>
class FSM : public std::vector<FSMState*> {
  public:
    typedef std::set<FSMState*> States;

    StateClass* addState() {
      StateClass* n = new StateClass();
      push_back(n);
      return n;
    }

    void dot() {
      printf("digraph foo {\n\trankdir=LR\n");
      for (iterator state_iter=begin(); 
          state_iter != end(); state_iter++) {
        FSMState* state = *state_iter;
        state->dot();
      }
      printf("}\n");
    }

    virtual ~FSM() {
      for (iterator state_iter=begin(); state_iter != end(); state_iter++) {
        delete *state_iter;
      }
    }
  protected:
    StateClass* mInitial;
};

#endif // __fsm_h__
