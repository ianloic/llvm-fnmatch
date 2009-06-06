#include "fnmatch-statemachine.h"

#include <string>
#include <vector>

using namespace fnmatch;

int State::stateNumCounter = 0;

// pattern an fnmatch pattern into a DFA
StateMachine::StateMachine(const std::string& pattern)
    : initial_state(NULL), last_state(NULL) {
  addState(State::Initial());
  for (size_t i=0; i<pattern.size(); i++) {
    switch(pattern[i]) {
      case '?':
        addState(State::Any());
        break;
      case '*':
        {
          State *s = State::Any();
          s->addChild(s); // multiple
          addState(s);
        }
        break;
      default:
        addState(State::Character(pattern[i]));
        break;
    }
  }
  addState(State::Match());
}

void
StateMachine::addState(State* state) {
  if (initial_state == NULL) {
    // track the first state
    initial_state = state;
  }
  states.push_back(state);
  if (last_state) {
    last_state->addChild(state);
  }
  last_state = state;
}

StateMachine::~StateMachine() {
  for (size_t i=0; i<states.size(); i++) {
    delete states[i];
  }
}


// graphviz generation
void
StateMachine::dot() {
  printf("digraph foo {\n\trankdir=LR\n");
  for (size_t i=0; i<states.size(); i++) {
    states[i]->dot();
  }
  printf("}\n");
}
void
State::dot() {
  for (size_t i=0; i<childStates.size(); i++) {
    childStates[i]->dotIn(stateNum);
  }
}
void
State::dotIn(int parentNum) {
  switch(kind) {
    case CHAR:
      printf("\ts%d -> s%d [label=\"%c\"]\n", parentNum, stateNum, c);
      break;
    case ANY:
      printf("\ts%d -> s%d [label=\"ANY\"]\n", parentNum, stateNum);
      break;
    case MATCH:
      printf("\ts%d -> s%d [label=\"MATCH\"]\n", parentNum, stateNum);
      break;
    case INITIAL:
      // uhh, shouldn't have arcs in
      return;
  }
}



int main(int argc, char** argv) {
  StateMachine *sm = new StateMachine(std::string(argv[1]));
  sm->dot();
}
