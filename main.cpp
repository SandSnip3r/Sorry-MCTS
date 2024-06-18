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

// int playFullGameMcts(mt19937 doActionEng) {
//   SorryMcts mcts = getSorryMcts();
//   Sorry state;
//   state.drawRandomStartingCards(doActionEng);
//   while (!state.gameDone()) {
//     // mcts.run(state, chrono::milliseconds(100));
//     // mcts.run(state, chrono::milliseconds(150));
//     // mcts.run(state, chrono::milliseconds(300));
//     // mcts.run(state, chrono::seconds(1));
//     // mcts.run(state, chrono::seconds(3));
//     // mcts.run(state, chrono::seconds(15));
//     // mcts.run(state, chrono::minutes(10));

//     // mcts.run(state, 10);
//     // mcts.run(state, 100);
//     mcts.run(state, 1000);
//     // mcts.run(state, 10000);
//     // mcts.run(state, 100000);
//     // mcts.run(state, 1000000);
//     Action bestAction = mcts.pickBestAction();
//     state.doAction(bestAction, doActionEng);
//   }
//   return state.getTotalActionCount();
// }

// int playFullGameRandomly(mt19937 doActionEng) {
//   mt19937 selectActionEng = createRandomEngine();
//   Sorry state;
//   state.drawRandomStartingCards(doActionEng);
//   while (!state.gameDone()) {
//     const auto actions = state.getActions();
//     uniform_int_distribution<int> dist(0, actions.size()-1);
//     const auto &action = actions.at(dist(selectActionEng));
//     state.doAction(action, doActionEng);
//   }
//   return state.getTotalActionCount();
// }

// void compare() {
//   constexpr const int kGameCount{100};
//   int randomTotalMoveCount=0;
//   int mctsTotalMoveCount=0;
//   for (int i=0; i<kGameCount; ++i) {
//     // Play an entire game completely randomly.
//     mt19937 eng(i);
//     const int randomMoveCount = playFullGameRandomly(eng);
//     randomTotalMoveCount += randomMoveCount;

//     // Play an entire game using MCTS.
//     const int mctsMoveCount = playFullGameMcts(eng);
//     mctsTotalMoveCount += mctsMoveCount;
//     cout << "Random game took " << randomMoveCount << " moves and MCTS game took " << mctsMoveCount << " moves" << endl;
//   }
//   cout << "Average random game length " << static_cast<double>(randomTotalMoveCount) / kGameCount << endl;
//   cout << "Average MCTS game length " << static_cast<double>(mctsTotalMoveCount) / kGameCount << endl;
// }

void aiVsAi() {
  mt19937 eng = createRandomEngine();

  // Who is playing?
  Sorry sorry({PlayerColor::kGreen, PlayerColor::kBlue});

  sorry.drawRandomStartingCards(eng);
  cout << "Drew cards" << endl;

  // Print starting board state.
  cout << "Starting state [size " << sizeof(sorry::Sorry) << "b]:" << endl;
  cout << "  " << sorry.toString() << endl;

  // Initialize AIs.
  SorryMcts greenMcts(/*explorationConstant=*/20.0);
  SorryMcts blueMcts(/*explorationConstant=*/20.0);
  const std::chrono::milliseconds kGreenTurnTimeLimit(1000);
  const std::chrono::milliseconds kBlueTurnTimeLimit(100);

  int turnNumber=0;
  while (!sorry.gameDone()) {
    // Who's turn?
    const PlayerColor currentTurn = sorry.getPlayerTurn();
    cout << "Turn #" << turnNumber << ", " << sorry::toString(currentTurn) << "'s" << endl;
    Action action;
    if (currentTurn == PlayerColor::kGreen) {
      greenMcts.run(sorry, kGreenTurnTimeLimit);
      action = greenMcts.pickBestAction();
    } else if (currentTurn == PlayerColor::kBlue) {
      blueMcts.run(sorry, kBlueTurnTimeLimit);
      action = blueMcts.pickBestAction();
    } else {
      throw std::runtime_error("Unknown player's turn");
    }
    cout << "Chose action " << action.toString() << endl;
    sorry.doAction(action, eng);
    cout << "Resulting state\n  " << sorry.toString() << std::endl;
    ++turnNumber;
  }
  std::cout << toString(sorry.getWinner()) << " wins!" << std::endl;
}

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
private:
  SorryMcts mcts_;
  int maxIterationCount_;
};

sorry::PlayerColor agentVsAgent(const std::map<sorry::PlayerColor, BaseAgent*> &agents) {
  mt19937 eng = createRandomEngine();
  std::vector<sorry::PlayerColor> playerColors;
  playerColors.reserve(agents.size());
  for (const auto &colorAndAgent : agents) {
    playerColors.push_back(colorAndAgent.first);
  }
  Sorry sorry(playerColors);
  sorry.drawRandomStartingCards(eng);

  int turnNumber=0;
  while (!sorry.gameDone()) {
    // Who's turn?
    const PlayerColor currentTurn = sorry.getPlayerTurn();
    BaseAgent *agent = agents.at(currentTurn);
    const sorry::Action action = agent->getAction(sorry);
    sorry.doAction(action, eng);
    ++turnNumber;
  }
  return sorry.getWinner();
}

void manualPlay() {
  mt19937 eng(123);

  // Who is playing?
  Sorry sorry({PlayerColor::kGreen, PlayerColor::kBlue});

  // Who goes first?
  // TODO: For now, we assume the first in the list given in the constructor.

  // What order do the player's take?
  // TODO: For now, we assume the order given in the constructor.

  // sorry.drawRandomStartingCards(eng);
  // cout << "Drew cards" << endl;

  // ----------- Debugging, start with specific cards & positions -----------
  // {Deck:1,(Green:Eight,One,Two,Four,Three|64,47,58,0),(Blue:Twelve,Twelve,One,Eight,Eleven|66,66,66,26)}
  sorry.setStartingCards(PlayerColor::kGreen, { Card::kEight, Card::kEleven, Card::kTwo, Card::kFour, Card::kThree });
  sorry.setStartingPositions(PlayerColor::kGreen, { 64,47,61,0 });
  sorry.setStartingCards(PlayerColor::kBlue, { Card::kTwelve, Card::kTwelve, Card::kOne, Card::kEight, Card::kEleven });
  sorry.setStartingPositions(PlayerColor::kBlue, { 66,66,66,26 });
  sorry.setTurn(PlayerColor::kGreen);

  // Print starting board state.
  cout << "Starting state [size " << sizeof(sorry::Sorry) << "b]:" << endl;
  cout << "  " << sorry.toString() << endl;

  int turnNumber=0;
  while (!sorry.gameDone()) {
    // Who's turn?
    const PlayerColor currentTurn = sorry.getPlayerTurn();
    cout << "Turn #" << turnNumber << ", " << sorry::toString(currentTurn) << "'s" << endl;
    const auto actions = sorry.getActions();
    for (size_t i=0; i<actions.size(); ++i) {
      cout << "  " << i << ": " << actions.at(i).toString() << endl;
    }
    size_t choice;
    cout << "Please choose: ";
    cin >> choice;
    sorry.doAction(actions.at(choice), eng);
    cout << "Resulting state:" << endl;
    cout << "  " << sorry.toString() << endl;
  }
  cout << "Game over. Winner is " << sorry::toString(sorry.getWinner()) << endl;
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
  // aiVsAi();
  // manualPlay();
  // doSingleMove();

  RandomAgent agent1;
  IterationBoundMctsAgent agent2(sqrt(2), 10);
  std::map<sorry::PlayerColor, BaseAgent*> agents = {{sorry::PlayerColor::kGreen, &agent1},
                                                     {sorry::PlayerColor::kBlue, &agent2}};
  std::map<sorry::PlayerColor, int> winCount;
  for (int i=0; i<200; ++i) {
    cout << "Playing game #" << i << endl;
    const sorry::PlayerColor winner = agentVsAgent(agents);
    ++winCount[winner];
  }
  for (const auto &colorCountPair : winCount) {
    std::cout << toString(colorCountPair.first) << " won " << colorCountPair.second << " time(s)" << std::endl;
  }
  return 0;
}