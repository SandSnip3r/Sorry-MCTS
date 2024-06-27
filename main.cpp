#include "common.hpp"
#include "sorry.hpp"
#include "sorryMcts.hpp"

#include <iostream>
#include <map>

using namespace sorry;
using namespace std;

// =============Board layout=============
//        30  32  34    37
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

class BaseAgent {
public:
  virtual sorry::Action getAction(const sorry::Sorry &state) = 0;
};

class RandomAgent : public BaseAgent {
public:
  RandomAgent() : eng_(createRandomEngine()) {}

  sorry::Action getAction(const sorry::Sorry &state) override {
    const auto actions = state.getActions();
    uniform_int_distribution<> dist(0, actions.size()-1);
    return actions.at(dist(eng_));
  }
private:
  std::mt19937 eng_;
};

class IterationBoundMctsAgent : public BaseAgent {
public:
  IterationBoundMctsAgent(double explorationConstant, int maxIterationCount) : mcts_(explorationConstant), maxIterationCount_(maxIterationCount) {}
  sorry::Action getAction(const sorry::Sorry &state) override {
    mcts_.run(state, maxIterationCount_);
    const sorry::Action action = mcts_.pickBestAction();
    mcts_.reset();
    return action;
  }
  SorryMcts& getMcts() { return mcts_; }
private:
  SorryMcts mcts_;
  int maxIterationCount_;
};

class HumanAgent : public BaseAgent {
public:
  HumanAgent() = default;
  sorry::Action getAction(const sorry::Sorry &state) override {
    cout << "State: " << state.toStringForCurrentPlayer() << endl;
    const auto actions = state.getActions();
    for (size_t i=0; i<actions.size(); ++i) {
      cout << "  " << i << ": " << actions.at(i).toString() << endl;
    }
    int choice = -1;
    while (choice < 0 || choice >= static_cast<int>(actions.size())) {
      cout << "Please choose [0-" << actions.size()-1 << "]: ";
      cin >> choice;
    }
    return actions.at(choice);
  }
private:
};

std::pair<bool, bool> agentVsAgent(BaseAgent &player1, BaseAgent &player2) {
  mt19937 firstGameEng = createRandomEngine();
  mt19937 secondGameEng = firstGameEng;

  Sorry firstSorryGameState({sorry::PlayerColor::kGreen, sorry::PlayerColor::kBlue});
  firstSorryGameState.drawRandomStartingCards(firstGameEng);
  Sorry secondSorryGameState = firstSorryGameState;

  while (!firstSorryGameState.gameDone()) {
    // Who's turn?
    const PlayerColor currentTurn = firstSorryGameState.getPlayerTurn();
    // In the first game, player 1 goes first (is green).
    BaseAgent &currentPlayer = (currentTurn == sorry::PlayerColor::kGreen ? player1 : player2);
    const sorry::Action action = currentPlayer.getAction(firstSorryGameState);
    firstSorryGameState.doAction(action, firstGameEng);
  }

  while (!secondSorryGameState.gameDone()) {
    // Who's turn?
    const PlayerColor currentTurn = secondSorryGameState.getPlayerTurn();
    // In the second game, player 2 goes first (is green).
    BaseAgent &currentPlayer = (currentTurn == sorry::PlayerColor::kGreen ? player2 : player1);
    const sorry::Action action = currentPlayer.getAction(secondSorryGameState);
    secondSorryGameState.doAction(action, secondGameEng);
  }

  std::cout << sorry::toString(firstSorryGameState.getWinner()) << " & " << sorry::toString(secondSorryGameState.getWinner()) << std::endl;
  return { (firstSorryGameState.getWinner() == sorry::PlayerColor::kGreen),
           (secondSorryGameState.getWinner() == sorry::PlayerColor::kBlue) };
}

void doSingleMove() {
  mt19937 eng(123);
  SorryMcts mcts(20.0);
  Sorry sorry({PlayerColor::kGreen, PlayerColor::kBlue});
  sorry.drawRandomStartingCards(eng);
  cout << "State " << sorry.toString() << std::endl;
  std::cout << "Actions are:" << std::endl;
  const auto actions = sorry.getActions();
  for (const auto &action : actions) {
    std::cout << "  " << action.toString() << std::endl;
  }
  mcts.run(sorry, 10000);
  Action bestAction = mcts.pickBestAction();
  std::cout << "Best action is " << bestAction.toString() << endl;
}

int main() {
  // HumanAgent agent1;
  IterationBoundMctsAgent agent1(0.65, 1000);
  IterationBoundMctsAgent agent2(0.65, 500);
  std::array<int, 2> winCount = { 0,0 };
  for (int i=0; i<10000; ++i) {
    const auto [agent1WonFirstGame, agent1WonSecondGame] = agentVsAgent(agent1, agent2);
    if (agent1WonFirstGame) {
      ++winCount[0];
    } else {
      ++winCount[1];
    }
    if (agent1WonSecondGame) {
      ++winCount[0];
    } else {
      ++winCount[1];
    }
    std::cout << "Player 1: " << winCount[0] << ", player 2: " << winCount[1] << std::endl;
  }
  return 0;
}