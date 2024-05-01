#include <iostream>

#include "sorry.hpp"
#include "sorryMcts.hpp"

using namespace std;

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
  return {explorationConstant_};
}

void doSingleMove() {
  Sorry s;
  s.setStartingCards({Card::kFour, Card::kTwelve, Card::kTwelve, Card::kTwelve, Card::kEleven});
  s.setStartingPositions({0,0,0,0});
  SorryMcts mcts = getSorryMcts();
  Action bestAction = mcts.pickBestAction(s, 400000);
  cout << "For state " << s.toString() << " best action is " << bestAction.toString() << endl;
}

void playFullGame() {
  Sorry s;
  s.setStartingCards({Card::kFive, Card::kTwelve, Card::kThree, Card::kTwelve, Card::kEleven});
  // s.setStartingPositions({0,0,0,0});
  // s.setStartingCards({Card::kSorry, Card::kSorry, Card::kSorry, Card::kSorry, Card::kEleven});
  mt19937 eng{0};
  // s.setStartingPositions({0,0,0,1});
  // s.setStartingPositions({13,31,0,60});
  cout << "Starting state: " << s.toString() << endl;
  SorryMcts mcts = getSorryMcts();
  while (!s.gameDone()) {
    // Action bestAction = mcts.pickBestAction(s, chrono::milliseconds(100));
    // Action bestAction = mcts.pickBestAction(s, chrono::milliseconds(150));
    // Action bestAction = mcts.pickBestAction(s, chrono::milliseconds(300));
    // Action bestAction = mcts.pickBestAction(s, chrono::seconds(1));
    // Action bestAction = mcts.pickBestAction(s, chrono::seconds(3));
    // Action bestAction = mcts.pickBestAction(s, chrono::seconds(15));
    // Action bestAction = mcts.pickBestAction(s, chrono::minutes(10));

    // Action bestAction = mcts.pickBestAction(s, 10000);
    // Action bestAction = mcts.pickBestAction(s, 100000);
    Action bestAction = mcts.pickBestAction(s, 1000000);
    s.doAction(bestAction, eng);
    cout << "Action " << bestAction.toString() << ". Resulting state " << s.toString() << endl;
    // cout << "For state " << s.toString() << " best action is " << bestAction.toString() << endl;
  }
  cout << "Game took " << s.getTotalActionCount() << " actions" << endl;
}

int main() {
  // doSingleMove();
  playFullGame();
  return 0;
}