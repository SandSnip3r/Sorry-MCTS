#include <iostream>

#include "common.hpp"
#include "sorry.hpp"
#include "sorryMcts.hpp"

using namespace sorry;
using namespace std;

// =============Board layout=============
//        30      34    37
// 28| |s|-|-|x| | | | |s|-|-|-|x| | |43
// 27| |                           |s|44
// 26|x|                           |.|45
// 25|.|                           |.|46
// 24|.|                           |x|47
// 23|.|                           | |48
// 22|s|                           | |49
// 21| |                           | |50
// 20| |                           | |51
// 19| |                     66|H| |s|52
// 18| |                       |.| |.|53
// 17|x|                     64|.| |.|54
// 16|.|                       |.| |.|55
// 15|.|                     62|.| |x|56
// 14|s|                   |S| |.| | |57
// 13| | |x|-|-|-|s| | | | |x|-|-|s| |58
//        11       7     4   2  60
// ======================================

SorryMcts getSorryMcts() {
  // constexpr double explorationConstant_ = 0.5;
  // constexpr double explorationConstant_ = 1;
  // constexpr double explorationConstant_ = sqrt(2);
  // constexpr double explorationConstant_ = 1.5;
  // constexpr double explorationConstant_ = 2;
  // constexpr double explorationConstant_ = 3;
  // constexpr double explorationConstant_ = 5;
  // constexpr double explorationConstant_ = 7.5;
  // constexpr double explorationConstant_ = 10;
  // constexpr double explorationConstant_ = 15;
  constexpr double explorationConstant_ = 20;
  // constexpr double explorationConstant_ = 50;
  // constexpr double explorationConstant_ = 100;
  // constexpr double explorationConstant_ = 200;
  // constexpr double explorationConstant_ = 300;
  // constexpr double explorationConstant_ = 400;
  return SorryMcts(explorationConstant_);
}

void doSingleMove() {
  Sorry s;
  s.setStartingCards({Card::kFour, Card::kTwelve, Card::kTwelve, Card::kTwelve, Card::kEleven});
  s.setStartingPositions({0,0,0,0});
  SorryMcts mcts = getSorryMcts();
  mcts.run(s, 400000);
  Action bestAction = mcts.pickBestAction();
  cout << "For state " << s.toString() << " best action is " << bestAction.toString() << endl;
}

int playFullGameMcts(mt19937 doActionEng) {
  SorryMcts mcts = getSorryMcts();
  Sorry state;
  state.drawRandomStartingCards(doActionEng);
  while (!state.gameDone()) {
    // mcts.run(state, chrono::milliseconds(100));
    // mcts.run(state, chrono::milliseconds(150));
    // mcts.run(state, chrono::milliseconds(300));
    // mcts.run(state, chrono::seconds(1));
    // mcts.run(state, chrono::seconds(3));
    // mcts.run(state, chrono::seconds(15));
    // mcts.run(state, chrono::minutes(10));

    // mcts.run(state, 10);
    // mcts.run(state, 100);
    mcts.run(state, 1000);
    // mcts.run(state, 10000);
    // mcts.run(state, 100000);
    // mcts.run(state, 1000000);
    Action bestAction = mcts.pickBestAction();
    state.doAction(bestAction, doActionEng);
  }
  return state.getTotalActionCount();
}

int playFullGameRandomly(mt19937 doActionEng) {
  mt19937 selectActionEng = createRandomEngine();
  Sorry state;
  state.drawRandomStartingCards(doActionEng);
  while (!state.gameDone()) {
    const auto actions = state.getActions();
    uniform_int_distribution<int> dist(0, actions.size()-1);
    const auto &action = actions.at(dist(selectActionEng));
    state.doAction(action, doActionEng);
  }
  return state.getTotalActionCount();
}

void compare() {
  constexpr const int kGameCount{100};
  int randomTotalMoveCount=0;
  int mctsTotalMoveCount=0;
  for (int i=0; i<kGameCount; ++i) {
    // Play an entire game completely randomly.
    mt19937 eng(i);
    const int randomMoveCount = playFullGameRandomly(eng);
    randomTotalMoveCount += randomMoveCount;

    // Play an entire game using MCTS.
    const int mctsMoveCount = playFullGameMcts(eng);
    mctsTotalMoveCount += mctsMoveCount;
    cout << "Random game took " << randomMoveCount << " moves and MCTS game took " << mctsMoveCount << " moves" << endl;
  }
  cout << "Average random game length " << static_cast<double>(randomTotalMoveCount) / kGameCount << endl;
  cout << "Average MCTS game length " << static_cast<double>(mctsTotalMoveCount) / kGameCount << endl;
}

int main() {
  compare();
  return 0;
}