// an NFA / DFA state machine implementation for pattern matching

#include <vector>
#include <string>

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

  // represents a finite state machine
  class StateMachine {
    public:
      explicit StateMachine(const std::string& pattern);
      ~StateMachine();

      void dot();

    private:
      // initial state in the machine
      State* initial_state;
      // all of the states in the machine
      std::vector<State*> states;
      // add a state to the machine
      inline void addState(State* state);
      // last state added
      State* last_state;
  };
}
