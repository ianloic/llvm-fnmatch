// an NFA / DFA state machine implementation for pattern matching

#include <vector>
#include <string>
#include <set>

#include <assert.h>

namespace fnmatch {
  typedef enum { 
    INITIAL, CHAR, ANY, MATCH
  } StateKind;

  // represents a DFA state
  class State {
    public:
      static State* Initial() {
        return new State(INITIAL);
      }
      static State* Character(int c) {
        State* s = new State(CHAR);
        s->c = c;
        return s;
      }
      static State* Any() {
        return new State(ANY);
      }
      // construct a MATCH state
      static State* Match() {
        return new State(MATCH);
      }

      void addChild(State* state) {
        childStates.push_back(state);
      }

      void dot();
      void dotIn(int parentNum);

    private:
      State(StateKind _kind) : kind(_kind) {
        stateNum = stateNumCounter++;
      }
      StateKind kind;
      int c;
      std::vector<State*> childStates;
      int stateNum;
      static int stateNumCounter;
  };

  typedef enum {
    CHARACTER, ALL, INCLUSIVE, EXCLUSIVE
  } CharacterSetType;
  class CharacterSet {
    public:
      CharacterSet(CharacterSetType _type, int _character)
        : type(_type), character(_character) {
          assert(_type == CHARACTER);
      }
      CharacterSet(CharacterSetType _type, std::set<int> _characterSet)
        : type(_type), characterSet(_characterSet) {
          assert(_type == INCLUSIVE || _type == EXCLUSIVE);
      }
      explicit CharacterSet(CharacterSetType _type) {
        assert(_type == ALL);
      }
      // operations we need
      CharacterSet* intersection(CharacterSet* other);
      CharacterSet* minus(CharacterSet* other);

      CharacterSetType type;
      int character;
      std::set<int> characterSet;
  };

  // represents a finite state machine
  class StateMachine {
    public:
      StateMachine();
      ~StateMachine();

      void dot();

    protected:
      // all of the states in the machine
      std::vector<State*> states;
      // add a state to the machine
      void addState(State* state);
  };

  class NFAStateMachine : public StateMachine {
    public:
      explicit NFAStateMachine(const std::string& pattern);
    private:
      // initial state in the machine
      State* initial_state;
      // last state added
      State* last_state;
      // add a state to the machine
      void addState(State* state);
  };
}
